#include "BlueprintCompilationManager.h"
#include "BlueprintValidator.h"
#include "Engine/Blueprint.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectGlobals.h"

class FPromptBeckonEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        FBlueprintCompilationManager::RegisterCompilerExtension( UBlueprint::StaticClass(), NewObject< UPromptBeckonBlueprintValidator >() );
    }
};

IMPLEMENT_MODULE( FPromptBeckonEditorModule, PromptBeckonEditor )
