#pragma once

#include "Components/SlateWrapperTypes.h"
#include "Containers/ArrayView.h"
#include "Containers/Set.h"
#include "Interaction/PBTypes.h"
#include "Misc/Optional.h"
#include "UObject/Object.h"
#include "UObject/WeakObjectPtr.h"
#include "Presenter.generated.h"

class APlayerController;
class UPromptBeckonDefinition;
class UPromptBeckonPointComponent;
class UUserWidget;

/**
 * Usage:
 * 1. Set its options in the ScreenPresenter or WorldPresenter slot of the interactor.
 * 2. Set Presenter in a definition to pick which one draws its prompt.
 */
UCLASS( Abstract, EditInlineNew, DefaultToInstanced )
class PROMPTBECKON_API UPromptBeckonPresenter : public UObject
{
    GENERATED_BODY()

public:
    virtual void Begin( APlayerController * Viewer ) PURE_VIRTUAL( UPromptBeckonPresenter::Begin, );
    virtual void Present( TConstArrayView< FPromptBeckonPromptView > Prompts, const FPromptBeckonFocusView & View, float DeltaSeconds ) PURE_VIRTUAL( UPromptBeckonPresenter::Present, );
    virtual void End() PURE_VIRTUAL( UPromptBeckonPresenter::End, );

    static float StepAlpha( float Alpha, bool bFocused, float DeltaSeconds, float InterpSpeed );
    static FPromptBeckonPromptState MakeState( const FPromptBeckonPromptView & View, float FocusAlpha );

protected:
    struct FWidgetChange
    {
        TWeakObjectPtr< UUserWidget > Widget;
        TOptional< ESlateVisibility > Visibility;
        TOptional< bool > bEnabled;
        TOptional< FPromptBeckonPromptState > State;
    };

    static const FPromptBeckonPromptView * FindPrompt( TConstArrayView< FPromptBeckonPromptView > Prompts, const TWeakObjectPtr< const UPromptBeckonPointComponent > & Point );
    static void TellWidgets( TConstArrayView< FWidgetChange > Changes );

    void ReportNoWidget( const UPromptBeckonDefinition * Definition );
    void ForgetReports();

private:
    TSet< TWeakObjectPtr< const UPromptBeckonDefinition > > ReportedDefs;
};
