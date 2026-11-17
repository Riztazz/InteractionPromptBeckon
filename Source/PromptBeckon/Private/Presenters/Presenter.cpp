#include "Interaction/Presenters/Presenter.h"

#include "Blueprint/UserWidget.h"
#include "Interaction/PBDefinition.h"
#include "Interaction/PBTypes.h"
#include "Interaction/PromptWidget.h"
#include "Logging/StructuredLog.h"
#include "Math/UnrealMathUtility.h"

namespace
{
    constexpr float SnapDist = 0.005f;
}

float UPromptBeckonPresenter::StepAlpha( float Alpha, bool bFocused, float DeltaSeconds, float InterpSpeed )
{
    const float Target  = bFocused ? 1.f : 0.f;
    const float Stepped = FMath::FInterpTo( Alpha, Target, DeltaSeconds, InterpSpeed );

    return FMath::Abs( Target - Stepped ) < SnapDist ? Target : Stepped;
}

FPromptBeckonPromptState UPromptBeckonPresenter::MakeState( const FPromptBeckonPromptView & View, float FocusAlpha )
{
    FPromptBeckonPromptState State;

    if ( View.Definition )
    {
        State.Verb = View.Definition->Verb;
    }

    State.Progress   = View.Progress;
    State.FocusAlpha = FocusAlpha;
    State.bEnabled   = View.bEnabled;

    return State;
}

const FPromptBeckonPromptView * UPromptBeckonPresenter::FindPrompt( TConstArrayView< FPromptBeckonPromptView > Prompts, const TWeakObjectPtr< const UPromptBeckonPointComponent > & Point )
{
    return Prompts.FindByPredicate( [ &Point ]( const FPromptBeckonPromptView & Prompt )
    {
        return Prompt.Point == Point;
    } );
}

void UPromptBeckonPresenter::TellWidgets( TConstArrayView< FWidgetChange > Changes )
{
    for ( const FWidgetChange & Change : Changes )
    {
        UUserWidget * Widget = Change.Widget.Get();
        if ( !Widget )
        {
            continue;
        }

        if ( Change.Visibility.IsSet() )
        {
            Widget->SetVisibility( Change.Visibility.GetValue() );
        }

        if ( Change.bEnabled.IsSet() )
        {
            Widget->SetIsEnabled( Change.bEnabled.GetValue() );
        }
    }

    for ( const FWidgetChange & Change : Changes )
    {
        UUserWidget * Widget = Change.Widget.Get();
        if ( Widget && Change.State.IsSet() )
        {
            IPromptBeckonPromptWidget::Execute_SetPromptState( Widget, Change.State.GetValue() );
        }
    }
}

void UPromptBeckonPresenter::ReportNoWidget( const UPromptBeckonDefinition * Definition )
{
    bool bReported = false;
    ReportedDefs.Add( Definition, &bReported );
    if ( !bReported )
    {
        UE_LOGFMT( LogPromptBeckon, Warning, "Definition '{Definition}' has no WidgetClass. Its prompts do not show.", Definition->GetName() );
    }
}

void UPromptBeckonPresenter::ForgetReports()
{
    ReportedDefs.Reset();
}
