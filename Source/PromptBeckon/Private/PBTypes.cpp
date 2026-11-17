#include "Interaction/PBTypes.h"

DEFINE_LOG_CATEGORY( LogPromptBeckon );

FPromptBeckonInteractionSessionId FPromptBeckonInteractionSessionId::Next()
{
    static int32 Counter = 0;

    FPromptBeckonInteractionSessionId Id;
    Id.Value = ++Counter;

    return Id;
}
