#pragma once

#include "Interaction/PBTypes.h"
#include "UObject/Interface.h"
#include "PromptWidget.generated.h"

/**
 * Usage:
 * 1. Add this interface to your prompt widget.
 * 2. Show the verb, progress and focus that SetPromptState gives you.
 * 3. Set the widget as the WidgetClass of a definition.
 */
UINTERFACE( MinimalAPI, Blueprintable )
class UPromptBeckonPromptWidget : public UInterface
{
    GENERATED_BODY()
};

class PROMPTBECKON_API IPromptBeckonPromptWidget
{
    GENERATED_BODY()

public:
    UFUNCTION( BlueprintNativeEvent, Category = "Prompt Beckon" )
    void SetPromptState( const FPromptBeckonPromptState & State );
};
