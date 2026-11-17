#pragma once

#include "Containers/Array.h"
#include "UObject/Interface.h"
#include "PointProvider.generated.h"

class UPromptBeckonPointComponent;

/**
 * Usage:
 * 1. Add this interface to every actor that has points.
 * 2. Return all of the actor's points from GetPoints.
 */
UINTERFACE( MinimalAPI, Blueprintable )
class UPromptBeckonPointProvider : public UInterface
{
    GENERATED_BODY()
};

class PROMPTBECKON_API IPromptBeckonPointProvider
{
    GENERATED_BODY()

public:
    UFUNCTION( BlueprintNativeEvent, BlueprintCallable, Category = "Prompt Beckon" )
    TArray< UPromptBeckonPointComponent * > GetPoints() const;
};
