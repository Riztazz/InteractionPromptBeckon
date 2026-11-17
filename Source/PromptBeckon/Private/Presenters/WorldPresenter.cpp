#include "Interaction/Presenters/WorldPresenter.h"

#include "Blueprint/UserWidget.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "Engine/CollisionProfile.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/PBDefinition.h"
#include "Interaction/PBTypes.h"
#include "Interaction/PromptWidget.h"
#include "Logging/StructuredLog.h"
#include "Materials/MaterialInterface.h"
#include "Math/Color.h"
#include "Math/Quat.h"
#include "Math/Rotator.h"
#include "Math/Transform.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector.h"
#include "Math/Vector2D.h"
#include "Templates/SubclassOf.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"

namespace
{
    const FVector2D Pivot( 0.5, 0.5 );

    FQuat FaceRot( const FRotator & ViewRot )
    {
        return ( ViewRot.Quaternion() * FQuat( FVector::UpVector, UE_PI ) ).GetNormalized();
    }

    UWidgetComponent * MakeComponent( AActor * Pool, UMaterialInterface * Material, TSubclassOf< UUserWidget > WidgetClass )
    {
        UWidgetComponent * Comp = NewObject< UWidgetComponent >( Pool );

        Comp->SetCollisionProfileName( UCollisionProfile::NoCollision_ProfileName );
        Comp->SetGenerateOverlapEvents( false );

        Comp->SetDrawAtDesiredSize( true );
        Comp->SetPivot( Pivot );

        Comp->SetBlendMode( EWidgetBlendMode::Transparent );

        Comp->SetAbsolute( true, true, true );

        if ( Material )
        {
            Comp->SetMaterial( 0, Material );
        }

        Comp->SetWidgetClass( WidgetClass );
        Comp->SetupAttachment( Pool->GetRootComponent() );
        Comp->RegisterComponent();

        return Comp;
    }
} // namespace

void UPromptBeckonWorldPresenter::Begin( APlayerController * Viewer )
{
    End();

    if ( !Viewer )
    {
        UE_LOGFMT( LogPromptBeckon, Error, "World presenter '{Presenter}' got no viewer. Nothing shows.", GetName() );

        return;
    }

    UWorld * World = Viewer->GetWorld();
    if ( !World )
    {
        UE_LOGFMT( LogPromptBeckon, Error, "World presenter '{Presenter}': viewer '{Viewer}' has no world. Nothing shows.", ( "Presenter", GetName() ), ( "Viewer", Viewer->GetName() ) );

        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.ObjectFlags                    = RF_Transient;
    SpawnParams.bDeferConstruction             = true;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
#if WITH_EDITOR
    SpawnParams.bHideFromSceneOutliner = true;
#endif

    AActor * Pool = World->SpawnActor< AActor >( AActor::StaticClass(), FTransform::Identity, SpawnParams );
    if ( !Pool )
    {
        UE_LOGFMT( LogPromptBeckon, Error, "World presenter '{Presenter}' could not spawn its pool actor in '{World}'. Nothing shows.", ( "Presenter", GetName() ), ( "World", World->GetName() ) );

        return;
    }

    Pool->SetRootComponent( NewObject< USceneComponent >( Pool, TEXT( "Root" ) ) );
    Pool->FinishSpawning( FTransform::Identity );

    PoolActor = Pool;
}

void UPromptBeckonWorldPresenter::Present( TConstArrayView< FPromptBeckonPromptView > Prompts, const FPromptBeckonFocusView & View, float DeltaSeconds )
{
    if ( !IsValid( PoolActor ) )
    {
        UE_LOGFMT( LogPromptBeckon, Verbose, "World presenter '{Presenter}' has no pool actor. Nothing shows.", GetName() );
        End();

        return;
    }

    Free.RemoveAll( []( const TObjectPtr< UWidgetComponent > & Comp )
    {
        return !IsValid( Comp );
    } );

    const auto IsDrawn = [ Prompts ]( const FSlot & Slot )
    {
        const UWidgetComponent * Comp = Slot.Comp.Get();
        if ( !Slot.Point.IsValid() || !Comp )
        {
            return false;
        }

        const FPromptBeckonPromptView * Prompt = FindPrompt( Prompts, Slot.Point );

        return Prompt && Prompt->Definition && Comp->GetWidgetClass() == Prompt->Definition->WidgetClass;
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

        UWidgetComponent * Comp = Acquire( Prompt.Definition->WidgetClass );
        if ( !IsValid( Comp ) )
        {
            continue;
        }

        FSlot & NewSlot = Slots.AddDefaulted_GetRef();
        NewSlot.Point   = Prompt.Point;
        NewSlot.Comp    = Comp;

        NewSlot.Sent.FocusAlpha = -1.f;
    }

    TArray< FWidgetChange > Pending;

    for ( int32 Idx = 0; Idx < Slots.Num(); ++Idx )
    {
        FSlot & Slot                           = Slots[ Idx ];
        UWidgetComponent * Comp                = Slot.Comp.Get();
        const FPromptBeckonPromptView * Prompt = FindPrompt( Prompts, Slot.Point );
        if ( !Comp || !Prompt || !Prompt->Definition )
        {
            continue;
        }

        const FPromptBeckonFocusLook & Look = Prompt->Definition->FocusLook;
        Slot.FocusAlpha                     = StepAlpha( Slot.FocusAlpha, Prompt->bFocused, DeltaSeconds, Look.InterpSpeed );

        if ( Slot.bFocused != Prompt->bFocused )
        {
            Slot.bFocused = Prompt->bFocused;
            Comp->SetTranslucentSortPriority( SortPriority + ( Slot.bFocused ? 1 : 0 ) );
        }

        Comp->SetWorldLocation( Prompt->Anchor.GetLocation() );
        Comp->SetWorldRotation( Prompt->Definition->bFaceCamera ? FaceRot( View.Rotation ) : Prompt->Anchor.GetRotation() );
        Comp->SetWorldScale3D( FVector( WorldScale * Look.ScaleAt( Slot.FocusAlpha ) ) );
        Comp->SetTintColorAndOpacity( FLinearColor( 1.f, 1.f, 1.f, Look.OpacityAt( Slot.FocusAlpha ) ) );

        UUserWidget * Widget = Comp->GetUserWidgetObject();
        if ( !Widget )
        {
            continue;
        }

        FWidgetChange Change;
        Change.Widget = Widget;
        if ( Widget->GetIsEnabled() != Prompt->bEnabled )
        {
            Change.bEnabled = Prompt->bEnabled;
        }

        const FPromptBeckonPromptState State = MakeState( *Prompt, Slot.FocusAlpha );
        if ( State != Slot.Sent && Widget->GetClass()->ImplementsInterface( UPromptBeckonPromptWidget::StaticClass() ) )
        {
            Slot.Sent    = State;
            Change.State = State;
        }

        if ( Change.bEnabled.IsSet() || Change.State.IsSet() )
        {
            Pending.Add( Change );
        }
    }

    TellWidgets( Pending );
}

void UPromptBeckonWorldPresenter::End()
{
    Slots.Reset();
    Free.Reset();
    ForgetReports();

    AActor * Doomed = PoolActor;
    PoolActor       = nullptr;

    if ( IsValid( Doomed ) )
    {
        Doomed->Destroy();
    }
}

int32 UPromptBeckonWorldPresenter::NumActive() const
{
    return Slots.Num();
}

int32 UPromptBeckonWorldPresenter::NumPooled() const
{
    return Free.Num();
}

UWidgetComponent * UPromptBeckonWorldPresenter::Acquire( TSubclassOf< UUserWidget > WidgetClass )
{
    int32 FreeIndex = Free.IndexOfByPredicate( [ &WidgetClass ]( const TObjectPtr< UWidgetComponent > & Cand )
    {
        return Cand->GetWidgetClass() == WidgetClass;
    } );
    if ( FreeIndex == INDEX_NONE && !Free.IsEmpty() )
    {
        FreeIndex = Free.Num() - 1;
    }

    UWidgetComponent * Comp = nullptr;
    if ( FreeIndex != INDEX_NONE )
    {
        Comp = Free[ FreeIndex ];
        Free.RemoveAtSwap( FreeIndex );

        if ( Comp->GetWidgetClass() != WidgetClass )
        {
            Comp->SetWidgetClass( WidgetClass );
        }
    }
    else
    {
        Comp = MakeComponent( PoolActor, PromptMaterial, WidgetClass );
    }

    Comp->SetTranslucentSortPriority( SortPriority );
    Comp->SetVisibility( true );
    Comp->SetComponentTickEnabled( true );

    return Comp;
}

void UPromptBeckonWorldPresenter::Release( FSlot & Slot )
{
    if ( UWidgetComponent * Comp = Slot.Comp.Get() )
    {
        Comp->SetVisibility( false );
        Comp->SetComponentTickEnabled( false );
        Free.Add( Comp );
    }

    Slot.Comp.Reset();
}
