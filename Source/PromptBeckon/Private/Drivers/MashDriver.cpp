#include "Interaction/Drivers/MashDriver.h"

#include "Interaction/InteractionTags.h"
#include "Math/UnrealMathUtility.h"

FPromptBeckonDriverStep FPromptBeckonMashDriver::Press( FPromptBeckonDriverRun & Run ) const
{
    ++Run.Presses;
    Run.SincePress = 0.f;

    return FPromptBeckonDriverStep{ Run.Presses >= PressCount ? EPromptBeckonInteractionStep::Complete : EPromptBeckonInteractionStep::Continue, FGameplayTag() };
}

FPromptBeckonDriverStep FPromptBeckonMashDriver::Release( FPromptBeckonDriverRun & Run ) const
{
    return FPromptBeckonDriverStep{ EPromptBeckonInteractionStep::Continue, FGameplayTag() };
}

FPromptBeckonDriverStep FPromptBeckonMashDriver::Advance( FPromptBeckonDriverRun & Run, float DeltaSeconds ) const
{
    Run.SincePress += DeltaSeconds;

    if ( Run.SincePress >= DecaySeconds )
    {
        return FPromptBeckonDriverStep{ EPromptBeckonInteractionStep::Abort, PromptBeckon::Tags::ResultCancelledDecayed };
    }

    return FPromptBeckonDriverStep{ EPromptBeckonInteractionStep::Continue, FGameplayTag() };
}

float FPromptBeckonMashDriver::GetProgress( const FPromptBeckonDriverRun & Run ) const
{
    return PressCount > 0 ? FMath::Clamp( static_cast< float >( Run.Presses ) / static_cast< float >( PressCount ), 0.f, 1.f ) : 0.f;
}
