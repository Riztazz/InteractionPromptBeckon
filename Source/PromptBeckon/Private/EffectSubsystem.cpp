#include "EffectSubsystem.h"

#include "Misc/AssertionMacros.h"

void UPromptBeckonEffectSubsystem::Deinitialize()
{
    ensureMsgf( !OwnerScope, TEXT( "World torn down while Prompt Beckon events are queued. They still go out." ) );
    OwnerScope = nullptr;

    Super::Deinitialize();
}
