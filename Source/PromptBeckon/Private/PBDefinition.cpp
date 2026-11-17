#include "Interaction/PBDefinition.h"

const FPromptBeckonDriver * UPromptBeckonDefinition::GetDriver() const
{
    return Driver.GetPtr();
}
