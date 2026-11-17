#pragma once

#include "Containers/Array.h"
#include "Interaction/PointComponent.h"
#include "UObject/NameTypes.h"

class UBlueprint;

struct FPromptBeckonPointProblem
{
    FName PointName;
    EPromptBeckonShapeProblem Problem = EPromptBeckonShapeProblem::None;
    bool bFromNative                  = false;
};

namespace PromptBeckonEditor
{
    PROMPTBECKONEDITOR_API void FindPointErrs( const UBlueprint * Blueprint, TArray< FPromptBeckonPointProblem > & OutErrs );
}
