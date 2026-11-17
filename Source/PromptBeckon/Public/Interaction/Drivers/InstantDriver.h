#pragma once

#include "Interaction/Drivers/InteractionDriver.h"
#include "InstantDriver.generated.h"

/**
 * Usage:
 * 1. Pick Instant as the Driver of a definition.
 * 2. One press then does the action.
 */
USTRUCT( DisplayName = "Instant" )
struct PROMPTBECKON_API FPromptBeckonInstantDriver : public FPromptBeckonDriver
{
    GENERATED_BODY()

    virtual FPromptBeckonDriverStep Press( FPromptBeckonDriverRun & Run ) const override;
    virtual FPromptBeckonDriverStep Release( FPromptBeckonDriverRun & Run ) const override;
    virtual FPromptBeckonDriverStep Advance( FPromptBeckonDriverRun & Run, float DeltaSeconds ) const override;
    virtual float GetProgress( const FPromptBeckonDriverRun & Run ) const override;
};
