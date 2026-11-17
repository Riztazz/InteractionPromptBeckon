#pragma once

#include "Engine/DataAsset.h"
#include "Interaction/Drivers/InteractionDriver.h"
#include "Interaction/PBTypes.h"
#include "Internationalization/Text.h"
#include "StructUtils/InstancedStruct.h"
#include "Templates/SubclassOf.h"
#include "PBDefinition.generated.h"

class UUserWidget;

/**
 * Usage:
 * 1. Make one data asset for each kind of use, such as Open or Pull.
 * 2. Pick the Driver, the Verb, the WidgetClass and the Presenter.
 * 3. Set it as the Definition of a point.
 */
UCLASS( BlueprintType )
class PROMPTBECKON_API UPromptBeckonDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY( EditAnywhere, Category = "Interaction", meta = ( ExcludeBaseStruct ) )
    TInstancedStruct< FPromptBeckonDriver > Driver;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = ( ClampMin = "0.0", ForceUnits = "s" ) )
    float CooldownSeconds = 0.f;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Prompt" )
    EPromptBeckonPresenterKind Presenter = EPromptBeckonPresenterKind::Screen;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Prompt" )
    TSubclassOf< UUserWidget > WidgetClass;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Prompt" )
    FText Verb;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Prompt" )
    FPromptBeckonFocusLook FocusLook;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Prompt" )
    bool bHideOnCooldown = true;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Prompt" )
    bool bFaceCamera = true;

    const FPromptBeckonDriver * GetDriver() const;
};
