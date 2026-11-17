#include "Interaction/Presenters/ScreenPresenter.h"

#include "Blueprint/UserWidget.h"
#include "Components/SlateWrapperTypes.h"
#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/PBDefinition.h"
#include "Interaction/PBTypes.h"
#include "Interaction/PromptWidget.h"
#include "Logging/StructuredLog.h"
#include "Math/Vector2D.h"
#include "Templates/SubclassOf.h"
#include "UObject/Class.h"

namespace
{
    const FVector2D Align( 0.5, 0.5 );

    bool AddToScreen( UUserWidget * Widget, int32 ZOrder )
    {
        const bool bAdded = Widget->AddToPlayerScreen( ZOrder );
        if ( !ensureMsgf( bAdded, TEXT( "Widget '%s' could not be added to the screen. The viewer must be a local player." ), *Widget->GetName() ) )
        {
            return false;
        }

        Widget->SetAlignmentInViewport( Align );

        return true;
    }
} // namespace

void UPromptBeckonScreenPresenter::Begin( APlayerController * InViewer )
{
    if ( !InViewer )
    {
        End();

        UE_LOGFMT( LogPromptBeckon, Error, "Screen presenter '{Presenter}' got no viewer. Nothing shows.", GetName() );

        return;
    }

    Viewer = InViewer;

    Pool.SetWorld( InViewer->GetWorld() );
    Pool.SetDefaultPlayerController( InViewer );
}

void UPromptBeckonScreenPresenter::Present( TConstArrayView< FPromptBeckonPromptView > Prompts, const FPromptBeckonFocusView & View, float DeltaSeconds )
{
    APlayerController * Player = Viewer.Get();
    if ( !Player )
    {
        UE_LOGFMT( LogPromptBeckon, Verbose, "Screen presenter '{Presenter}' has no viewer. Nothing shows.", GetName() );
        End();

        return;
    }

    const auto IsDrawn = [ Prompts ]( const FSlot & Slot )
    {
        const UUserWidget * Widget = Slot.Widget.Get();
        if ( !Slot.Point.IsValid() || !Widget )
        {
            return false;
        }

        const FPromptBeckonPromptView * Prompt = FindPrompt( Prompts, Slot.Point );

        return Prompt && Prompt->Definition && Widget->GetClass() == Prompt->Definition->WidgetClass;
    };

    for ( int32 Idx = Slots.Num() - 1; Idx >= 0; --Idx )
    {
        if ( !IsDrawn( Slots[ Idx ] ) )
        {
            FSlot Dropped = MoveTemp( Slots[ Idx ] );
            Slots.RemoveAtSwap( Idx );
            Release( Dropped );
        }
    }

    for ( const FPromptBeckonPromptView & Prompt : Prompts )
    {
        if ( !Prompt.Point.IsValid() || !Prompt.Definition )
        {
            continue;
        }

        const bool bHasSlot = Slots.ContainsByPredicate( [ &Prompt ]( const FSlot & Slot )
        {
            return Slot.Point == Prompt.Point;
        } );
        if ( bHasSlot )
        {
            continue;
        }

        if ( !Prompt.Definition->WidgetClass )
        {
            ReportNoWidget( Prompt.Definition );

            continue;
        }

        UUserWidget * Widget = Pool.GetOrCreateInstance( Prompt.Definition->WidgetClass );
        if ( !Widget )
        {
            UE_LOGFMT( LogPromptBeckon, Error, "Definition '{Definition}' could not make widget '{Class}'. Its prompts do not show.", ( "Definition", Prompt.Definition->GetName() ), ( "Class", Prompt.Definition->WidgetClass->GetName() ) );

            continue;
        }

        if ( !AddToScreen( Widget, ZOrder + ( Prompt.bFocused ? 1 : 0 ) ) )
        {
            Pool.Release( Widget );

            continue;
        }

        FSlot & NewSlot  = Slots.AddDefaulted_GetRef();
        NewSlot.Point    = Prompt.Point;
        NewSlot.Widget   = Widget;
        NewSlot.bFocused = Prompt.bFocused;

        NewSlot.Sent.FocusAlpha = -1.f;
    }

    int32 ViewW = 0;
    int32 ViewH = 0;
    Player->GetViewportSize( ViewW, ViewH );

    TArray< FWidgetChange > Pending;

    for ( int32 Idx = 0; Idx < Slots.Num(); ++Idx )
    {
        FSlot & Slot                           = Slots[ Idx ];
        UUserWidget * Widget                   = Slot.Widget.Get();
        const FPromptBeckonPromptView * Prompt = FindPrompt( Prompts, Slot.Point );
        if ( !Widget || !Prompt || !Prompt->Definition )
        {
            continue;
        }

        const FPromptBeckonFocusLook & Look = Prompt->Definition->FocusLook;
        Slot.FocusAlpha                     = StepAlpha( Slot.FocusAlpha, Prompt->bFocused, DeltaSeconds, Look.InterpSpeed );

        const bool bFocusChanged = Slot.bFocused != Prompt->bFocused;
        Slot.bFocused            = Prompt->bFocused;

        FWidgetChange Change;
        Change.Widget = Widget;

        const FPromptBeckonPromptState State = MakeState( *Prompt, Slot.FocusAlpha );
        if ( State != Slot.Sent && Widget->GetClass()->ImplementsInterface( UPromptBeckonPromptWidget::StaticClass() ) )
        {
            Slot.Sent    = State;
            Change.State = State;
        }

        Widget->SetRenderScale( FVector2D( Look.ScaleAt( Slot.FocusAlpha ) ) );
        Widget->SetRenderOpacity( Look.OpacityAt( Slot.FocusAlpha ) );
        Widget->SetIsEnabled( Prompt->bEnabled );

        if ( bFocusChanged )
        {
            Widget->RemoveFromParent();
            AddToScreen( Widget, ZOrder + ( Prompt->bFocused ? 1 : 0 ) );
        }

        FVector2D Position    = FVector2D::ZeroVector;
        const bool bProjected = Player->ProjectWorldLocationToScreen( Prompt->Anchor.GetLocation(), Position, false );
        const bool bOnScreen  = bProjected && Position.X >= 0. && Position.Y >= 0. && Position.X <= ViewW && Position.Y <= ViewH;

        const ESlateVisibility Visibility = bOnScreen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
        if ( Widget->GetVisibility() != Visibility )
        {
            Change.Visibility = Visibility;
        }

        if ( bOnScreen )
        {
            Widget->SetPositionInViewport( Position, true );
        }

        if ( Change.Visibility.IsSet() || Change.State.IsSet() )
        {
            Pending.Add( Change );
        }
    }

    TellWidgets( Pending );
}

void UPromptBeckonScreenPresenter::End()
{
    TArray< FSlot > Ended = MoveTemp( Slots );
    Slots.Reset();

    for ( FSlot & Slot : Ended )
    {
        Release( Slot );
    }

    Pool.ResetPool();
    Pool.SetWorld( nullptr );
    Pool.SetDefaultPlayerController( nullptr );

    ForgetReports();
    Viewer.Reset();
}

int32 UPromptBeckonScreenPresenter::NumActive() const
{
    return Slots.Num();
}

void UPromptBeckonScreenPresenter::Release( FSlot & Slot )
{
    if ( UUserWidget * Widget = Slot.Widget.Get() )
    {
        Widget->RemoveFromParent();
        Pool.Release( Widget );
    }

    Slot.Widget.Reset();
}
