#pragma once

#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "InteractionSettings.generated.h"

/**
 * Usage:
 * 1. Open Project Settings, Prompt Beckon.
 * 2. Set DetectChannel to the channel your detection shapes overlap.
 * 3. Set OccludeChannel to the channel that blocks line of sight.
 */
UCLASS( Config = Game, DefaultConfig, meta = ( DisplayName = "Prompt Beckon" ) )
class PROMPTBECKON_API UPromptBeckonInteractionSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY( EditAnywhere, Config, Category = "Detection" )
    TEnumAsByte< ECollisionChannel > OccludeChannel = ECC_Visibility;

    UPROPERTY( EditAnywhere, Config, Category = "Detection", meta = ( ConfigRestartRequired = true ) )
    TEnumAsByte< ECollisionChannel > DetectChannel = ECC_GameTraceChannel2;
};
