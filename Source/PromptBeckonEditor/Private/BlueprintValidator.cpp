#include "BlueprintValidator.h"

#include "BlueprintValidation.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Engine/Blueprint.h"
#include "Interaction/PointComponent.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "KismetCompiler.h"
#include "UObject/Class.h"

namespace
{
    FString NativeNameOf( const UBlueprint * Blueprint )
    {
        const UClass * NativeClass = Blueprint ? FBlueprintEditorUtils::FindFirstNativeClass( Blueprint->ParentClass.Get() ) : nullptr;

        return NativeClass ? FString( NativeClass->GetPrefixCPP() ) + NativeClass->GetName() : FString( TEXT( "None" ) );
    }
} // namespace

void UPromptBeckonBlueprintValidator::ProcessBlueprintCompiled( const FKismetCompilerContext & CompilationContext, const FBlueprintCompiledData & Data )
{
    const UBlueprint * Blueprint = CompilationContext.Blueprint;

    TArray< FPromptBeckonPointProblem > Problems;
    PromptBeckonEditor::FindPointErrs( Blueprint, Problems );

    for ( const FPromptBeckonPointProblem & Entry : Problems )
    {
        const FString ErrText = UPromptBeckonPointComponent::GetShapeErrText( Entry.Problem ).ToString();

        if ( Entry.bFromNative )
        {
            CompilationContext.MessageLog.Warning( *FString::Printf( TEXT( "Prompt Beckon Point '%s' (fix it in C++ class %s): %s" ), *Entry.PointName.ToString(), *NativeNameOf( Blueprint ), *ErrText ) );
        }
        else if ( UPromptBeckonPointComponent::IsShapeErr( Entry.Problem ) )
        {
            CompilationContext.MessageLog.Error( *FString::Printf( TEXT( "Prompt Beckon Point '%s': %s" ), *Entry.PointName.ToString(), *ErrText ) );
        }
        else
        {
            CompilationContext.MessageLog.Warning( *FString::Printf( TEXT( "Prompt Beckon Point '%s': %s" ), *Entry.PointName.ToString(), *ErrText ) );
        }
    }
}
