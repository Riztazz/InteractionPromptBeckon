#pragma once

#include "BlueprintCompilerExtension.h"
#include "BlueprintValidator.generated.h"

UCLASS()
class UPromptBeckonBlueprintValidator : public UBlueprintCompilerExtension
{
    GENERATED_BODY()

protected:
    virtual void ProcessBlueprintCompiled( const FKismetCompilerContext & CompilationContext, const FBlueprintCompiledData & Data ) override;
};
