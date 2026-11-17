#include "Interaction/Drivers/HoldDriver.h"

#include "Interaction/InteractionTags.h"
#include "Math/UnrealMathUtility.h"

FPromptBeckonDriverStep FPromptBeckonHoldDriver::Press( FPromptBeckonDriverRun & Run ) const
{
    if ( !Run.bHeld )
    {
        Run.bHeld   = true;
        Run.Elapsed = 0.f;
    }

    return FPromptBeckonDriverStep{ EPromptBeckonInteractionStep::Continue, FGameplayTag() };
}

FPromptBeckonDriverStep FPromptBeckonHoldDriver::Release( FPromptBeckonDriverRun & Run ) const
{
    if ( Run.bHeld && Run.Elapsed < DurationSeconds )
    {
        Run.bHeld = false;

        return FPromptBeckonDriverStep{ EPromptBeckonInteractionStep::Abort, PromptBeckon::Tags::ResultCancelledReleased };
    }

    Run.bHeld = false;

    return FPromptBeckonDriverStep{ EPromptBeckonInteractionStep::Continue, FGameplayTag() };
}

FPromptBeckonDriverStep FPromptBeckonHoldDriver::Advance( FPromptBeckonDriverRun & Run, float DeltaSeconds ) const
{
    if ( !Run.bHeld )
    {
        return FPromptBeckonDriverStep{ EPromptBeckonInteractionStep::Continue, FGameplayTag() };
    }

    Run.Elapsed += DeltaSeconds;

    return FPromptBeckonDriverStep{ Run.Elapsed >= DurationSeconds ? EPromptBeckonInteractionStep::Complete : EPromptBeckonInteractionStep::Continue, FGameplayTag() };
}

float FPromptBeckonHoldDriver::GetProgress( const FPromptBeckonDriverRun & Run ) const
{
    return DurationSeconds > 0.f ? FMath::Clamp( Run.Elapsed / DurationSeconds, 0.f, 1.f ) : 0.f;
}
