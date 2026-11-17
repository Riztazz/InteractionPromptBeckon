#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "EffectSubsystem.generated.h"

class FPromptBeckonEffectScope;

/** Purpose: knows the outermost open effect scope of its world, so a nested scope in the same world joins its queue. */
UCLASS()
class UPromptBeckonEffectSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

private:
    friend class FPromptBeckonEffectScope;

    FPromptBeckonEffectScope * OwnerScope = nullptr;
};
