#pragma once

#include "Interaction/Drivers/InteractionDriver.h"
#include "MashDriver.generated.h"

/**
 * Usage:
 * 1. Pick Mash as the Driver of a definition.
 * 2. Set PressCount to the number of presses needed.
 * 3. Set DecaySeconds to the longest wait allowed between presses.
 */
USTRUCT( DisplayName = "Mash" )
struct PROMPTBECKON_API FPromptBeckonMashDriver : public FPromptBeckonDriver
{
    GENERATED_BODY()

    virtual FPromptBeckonDriverStep Press( FPromptBeckonDriverRun & Run ) const override;
    virtual FPromptBeckonDriverStep Release( FPromptBeckonDriverRun & Run ) const override;
    virtual FPromptBeckonDriverStep Advance( FPromptBeckonDriverRun & Run, float DeltaSeconds ) const override;
    virtual float GetProgress( const FPromptBeckonDriverRun & Run ) const override;

    UPROPERTY( EditAnywhere, Category = "Mash", meta = ( ClampMin = "2" ) )
    int32 PressCount = 5;

    UPROPERTY( EditAnywhere, Category = "Mash", meta = ( ClampMin = "0.05", ForceUnits = "s" ) )
    float DecaySeconds = 1.f;
};
