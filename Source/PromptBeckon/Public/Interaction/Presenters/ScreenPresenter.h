#pragma once

#include "Blueprint/UserWidgetPool.h"
#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "Interaction/PBTypes.h"
#include "Interaction/Presenters/Presenter.h"
#include "UObject/WeakObjectPtr.h"
#include "ScreenPresenter.generated.h"

class APlayerController;
class UPromptBeckonPointComponent;
class UUserWidget;

/**
 * Usage:
 * 1. Set Presenter to Screen in a definition.
 * 2. Its prompts then draw flat on the screen, over the point.
 */
UCLASS( DisplayName = "Screen" )
class PROMPTBECKON_API UPromptBeckonScreenPresenter : public UPromptBeckonPresenter
{
    GENERATED_BODY()

public:
    virtual void Begin( APlayerController * InViewer ) override;
    virtual void Present( TConstArrayView< FPromptBeckonPromptView > Prompts, const FPromptBeckonFocusView & View, float DeltaSeconds ) override;
    virtual void End() override;

    UPROPERTY( EditAnywhere, Category = "Prompt Beckon" )
    int32 ZOrder = 0;

    int32 NumActive() const;

private:
    /** Purpose: one live prompt on screen, its point, its widget and what that widget last showed. */
    struct FSlot
    {
        TWeakObjectPtr< const UPromptBeckonPointComponent > Point;
        TWeakObjectPtr< UUserWidget > Widget;
        float FocusAlpha = 0.f;
        bool bFocused    = false;
        FPromptBeckonPromptState Sent;
    };

    UPROPERTY( Transient )
    FUserWidgetPool Pool;

    TArray< FSlot > Slots;
    TWeakObjectPtr< APlayerController > Viewer;

    void Release( FSlot & Slot );
};
