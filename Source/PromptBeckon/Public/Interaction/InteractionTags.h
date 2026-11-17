#pragma once

#include "NativeGameplayTags.h"

namespace PromptBeckon::Tags
{
    PROMPTBECKON_API UE_DECLARE_GAMEPLAY_TAG_EXTERN( ResultCompleted );
    PROMPTBECKON_API UE_DECLARE_GAMEPLAY_TAG_EXTERN( ResultCancelledReleased );
    PROMPTBECKON_API UE_DECLARE_GAMEPLAY_TAG_EXTERN( ResultCancelledDecayed );
    PROMPTBECKON_API UE_DECLARE_GAMEPLAY_TAG_EXTERN( ResultAbortedTargetDestroyed );
    PROMPTBECKON_API UE_DECLARE_GAMEPLAY_TAG_EXTERN( ResultAbortedInteractorDestroyed );
    PROMPTBECKON_API UE_DECLARE_GAMEPLAY_TAG_EXTERN( ResultAbortedUnavailable );
    PROMPTBECKON_API UE_DECLARE_GAMEPLAY_TAG_EXTERN( ResultAbortedReset );
    PROMPTBECKON_API UE_DECLARE_GAMEPLAY_TAG_EXTERN( ResultCancelledOutOfRange );
    PROMPTBECKON_API UE_DECLARE_GAMEPLAY_TAG_EXTERN( ResultCancelledSuspended );
    PROMPTBECKON_API UE_DECLARE_GAMEPLAY_TAG_EXTERN( ResultCancelledViewerLost );
} // namespace PromptBeckon::Tags
