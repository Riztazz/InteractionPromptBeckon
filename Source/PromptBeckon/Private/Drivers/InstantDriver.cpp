#include "Interaction/Drivers/InstantDriver.h"

FPromptBeckonDriverStep FPromptBeckonInstantDriver::Press( FPromptBeckonDriverRun & Run ) const
{
    return FPromptBeckonDriverStep{ EPromptBeckonInteractionStep::Complete, FGameplayTag() };
}

FPromptBeckonDriverStep FPromptBeckonInstantDriver::Release( FPromptBeckonDriverRun & Run ) const
{
    return FPromptBeckonDriverStep{ EPromptBeckonInteractionStep::Continue, FGameplayTag() };
}

FPromptBeckonDriverStep FPromptBeckonInstantDriver::Advance( FPromptBeckonDriverRun & Run, float DeltaSeconds ) const
{
    return FPromptBeckonDriverStep{ EPromptBeckonInteractionStep::Continue, FGameplayTag() };
}

float FPromptBeckonInstantDriver::GetProgress( const FPromptBeckonDriverRun & Run ) const
{
    return 0.f;
}
