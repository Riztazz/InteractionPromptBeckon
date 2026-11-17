#include "Interaction/InteractionTags.h"

namespace PromptBeckon::Tags
{
    UE_DEFINE_GAMEPLAY_TAG_COMMENT( ResultCompleted, "Interaction.Result.Completed", "The interaction performed" );
    UE_DEFINE_GAMEPLAY_TAG_COMMENT( ResultCancelledReleased, "Interaction.Result.Cancelled.Released", "A hold was let go before its duration" );
    UE_DEFINE_GAMEPLAY_TAG_COMMENT( ResultCancelledDecayed, "Interaction.Result.Cancelled.Decayed", "A mash went too long without a press" );
    UE_DEFINE_GAMEPLAY_TAG_COMMENT( ResultAbortedTargetDestroyed, "Interaction.Result.Aborted.TargetDestroyed", "The target was destroyed mid-interaction" );
    UE_DEFINE_GAMEPLAY_TAG_COMMENT( ResultAbortedInteractorDestroyed, "Interaction.Result.Aborted.InteractorDestroyed", "The interactor left play mid-interaction" );
    UE_DEFINE_GAMEPLAY_TAG_COMMENT( ResultAbortedUnavailable, "Interaction.Result.Aborted.Unavailable", "The owner refused or hid the target mid-interaction" );
    UE_DEFINE_GAMEPLAY_TAG_COMMENT( ResultAbortedReset, "Interaction.Result.Aborted.Reset", "The target was reset to available mid-interaction" );
    UE_DEFINE_GAMEPLAY_TAG_COMMENT( ResultCancelledOutOfRange, "Interaction.Result.Cancelled.OutOfRange", "The interactor moved out of range" );
    UE_DEFINE_GAMEPLAY_TAG_COMMENT( ResultCancelledSuspended, "Interaction.Result.Cancelled.Suspended", "The interactor was suspended" );
    UE_DEFINE_GAMEPLAY_TAG_COMMENT( ResultCancelledViewerLost, "Interaction.Result.Cancelled.ViewerLost", "The pawn lost its local player controller" );
} // namespace PromptBeckon::Tags
