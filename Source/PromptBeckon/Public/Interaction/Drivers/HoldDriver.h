#pragma once

#include "Interaction/Drivers/InteractionDriver.h"
#include "HoldDriver.generated.h"

/**
 * Usage:
 * 1. Pick Hold as the Driver of a definition.
 * 2. Set DurationSeconds to how long the key must stay down.
 */
USTRUCT( DisplayName = "Hold" )
struct PROMPTBECKON_API FPromptBeckonHoldDriver : public FPromptBeckonDriver
{
    GENERATED_BODY()

    virtual FPromptBeckonDriverStep Press( FPromptBeckonDriverRun & Run ) const override;
    virtual FPromptBeckonDriverStep Release( FPromptBeckonDriverRun & Run ) const override;
    virtual FPromptBeckonDriverStep Advance( FPromptBeckonDriverRun & Run, float DeltaSeconds ) const override;
    virtual float GetProgress( const FPromptBeckonDriverRun & Run ) const override;

    UPROPERTY( EditAnywhere, Category = "Hold", meta = ( ClampMin = "0.05", ForceUnits = "s" ) )
    float DurationSeconds = 1.f;
};
