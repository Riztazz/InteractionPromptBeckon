#pragma once

#include "GameplayTagContainer.h"
#include "UObject/ObjectMacros.h"
#include "InteractionDriver.generated.h"

UENUM()
enum class EPromptBeckonInteractionStep : uint8
{
    Continue,

    Complete,

    Abort
};

USTRUCT()
struct FPromptBeckonDriverRun
{
    GENERATED_BODY()

    float Elapsed    = 0.f;
    float SincePress = 0.f;
    int32 Presses    = 0;
    bool bHeld       = false;
};

USTRUCT()
struct FPromptBeckonDriverStep
{
    GENERATED_BODY()

    EPromptBeckonInteractionStep Step = EPromptBeckonInteractionStep::Continue;
    FGameplayTag AbortReason;
};

/**
 * Usage:
 * 1. Derive a struct from it to make a new kind of use.
 * 2. Override Press, Release, Advance and GetProgress.
 * 3. Pick it as the Driver of a definition.
 */
USTRUCT()
struct PROMPTBECKON_API FPromptBeckonDriver
{
    GENERATED_BODY()

    virtual ~FPromptBeckonDriver() = default;

    virtual FPromptBeckonDriverStep Press( FPromptBeckonDriverRun & Run ) const
    {
        return FPromptBeckonDriverStep{ EPromptBeckonInteractionStep::Abort, FGameplayTag() };
    }

    virtual FPromptBeckonDriverStep Release( FPromptBeckonDriverRun & Run ) const
    {
        return FPromptBeckonDriverStep{ EPromptBeckonInteractionStep::Abort, FGameplayTag() };
    }

    virtual FPromptBeckonDriverStep Advance( FPromptBeckonDriverRun & Run, float DeltaSeconds ) const
    {
        return FPromptBeckonDriverStep{ EPromptBeckonInteractionStep::Abort, FGameplayTag() };
    }

    virtual float GetProgress( const FPromptBeckonDriverRun & Run ) const
    {
        return 0.f;
    }
};
