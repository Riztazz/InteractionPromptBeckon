#pragma once

#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "Interaction/PBTypes.h"
#include "Interaction/Presenters/Presenter.h"
#include "Templates/SubclassOf.h"
#include "UObject/ObjectPtr.h"
#include "UObject/WeakObjectPtr.h"
#include "WorldPresenter.generated.h"

class AActor;
class APlayerController;
class UMaterialInterface;
class UPromptBeckonPointComponent;
class UUserWidget;
class UWidgetComponent;

/**
 * Usage:
 * 1. Set Presenter to World in a definition.
 * 2. Its prompts then draw in the world, so walls can hide them.
 */
UCLASS( DisplayName = "World" )
class PROMPTBECKON_API UPromptBeckonWorldPresenter : public UPromptBeckonPresenter
{
    GENERATED_BODY()

public:
    virtual void Begin( APlayerController * Viewer ) override;
    virtual void Present( TConstArrayView< FPromptBeckonPromptView > Prompts, const FPromptBeckonFocusView & View, float DeltaSeconds ) override;
    virtual void End() override;

    UPROPERTY( EditAnywhere, Category = "Prompt Beckon" )
    TObjectPtr< UMaterialInterface > PromptMaterial;

    UPROPERTY( EditAnywhere, Category = "Prompt Beckon" )
    int32 SortPriority = 10;

    UPROPERTY( EditAnywhere, Category = "Prompt Beckon", meta = ( ClampMin = "0.001" ) )
    float WorldScale = 0.1f;

    int32 NumActive() const;
    int32 NumPooled() const;

private:
    /** Purpose: one live prompt in the world, its point, its widget component and what its widget last showed. */
    struct FSlot
    {
        TWeakObjectPtr< const UPromptBeckonPointComponent > Point;
        TWeakObjectPtr< UWidgetComponent > Comp;
        float FocusAlpha = 0.f;
        bool bFocused    = false;
        FPromptBeckonPromptState Sent;
    };

    UPROPERTY( Transient )
    TObjectPtr< AActor > PoolActor;

    UPROPERTY( Transient )
    TArray< TObjectPtr< UWidgetComponent > > Free;

    TArray< FSlot > Slots;

    UWidgetComponent * Acquire( TSubclassOf< UUserWidget > WidgetClass );
    void Release( FSlot & Slot );
};
