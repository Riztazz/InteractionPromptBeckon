#pragma once

#include "UObject/Interface.h"
#include "InteractionGate.generated.h"

class UPromptBeckonInteractorComponent;
class UPromptBeckonPointComponent;

/**
 * Usage:
 * 1. Add this interface to the actor of a point, or to one of its components.
 * 2. Return false from CanBegin to refuse a press.
 */
UINTERFACE( MinimalAPI, Blueprintable )
class UPromptBeckonInteractionGate : public UInterface
{
    GENERATED_BODY()
};

class PROMPTBECKON_API IPromptBeckonInteractionGate
{
    GENERATED_BODY()

public:
    UFUNCTION( BlueprintNativeEvent, Category = "Prompt Beckon" )
    bool CanBegin( const UPromptBeckonPointComponent * Point, const UPromptBeckonInteractorComponent * Interactor ) const;
};
