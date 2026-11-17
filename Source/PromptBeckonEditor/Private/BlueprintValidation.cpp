#include "BlueprintValidation.h"

#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "GameFramework/Actor.h"
#include "Interaction/PointProvider.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "UObject/Class.h"

namespace
{
    const USceneComponent * FindTplByVar( const USimpleConstructionScript * Script, FName VarName )
    {
        if ( !Script )
        {
            return nullptr;
        }

        for ( const USCS_Node * Node : Script->GetAllNodes() )
        {
            if ( Node && Node->GetVariableName() == VarName )
            {
                return Cast< USceneComponent >( Node->ComponentTemplate );
            }
        }

        return nullptr;
    }

    const USceneComponent * FindComponentByName( const AActor * Defaults, FName CompName )
    {
        if ( !Defaults )
        {
            return nullptr;
        }

        for ( const UActorComponent * Comp : Defaults->GetComponents() )
        {
            if ( Comp && Comp->GetFName() == CompName )
            {
                return Cast< USceneComponent >( Comp );
            }
        }

        return nullptr;
    }

    const AActor * FindNativeCDO( const UBlueprint * Blueprint )
    {
        if ( !Blueprint )
        {
            return nullptr;
        }

        const UClass * NativeClass = FBlueprintEditorUtils::FindFirstNativeClass( Blueprint->ParentClass.Get() );

        return NativeClass ? Cast< AActor >( NativeClass->GetDefaultObject( false ) ) : nullptr;
    }

    TArray< const USimpleConstructionScript * > FindParentScs( const UBlueprint * Blueprint )
    {
        TArray< const USimpleConstructionScript * > Scripts;
        if ( !Blueprint )
        {
            return Scripts;
        }

        for ( const UClass * Class = Blueprint->ParentClass.Get(); Class && !Class->HasAnyClassFlags( CLASS_Native ); Class = Class->GetSuperClass() )
        {
            const UBlueprintGeneratedClass * BPClass = Cast< UBlueprintGeneratedClass >( Class );
            if ( BPClass && BPClass->SimpleConstructionScript )
            {
                Scripts.Add( BPClass->SimpleConstructionScript );
            }
        }

        return Scripts;
    }

    const USceneComponent * FindInheritedTpl( const UBlueprint * Blueprint, const USCS_Node * Node )
    {
        if ( !Node || Node->ParentComponentOrVariableName == NAME_None )
        {
            return nullptr;
        }

        if ( Node->bIsParentComponentNative )
        {
            return FindComponentByName( FindNativeCDO( Blueprint ), Node->ParentComponentOrVariableName );
        }

        for ( const USimpleConstructionScript * Script : FindParentScs( Blueprint ) )
        {
            if ( const USceneComponent * Found = FindTplByVar( Script, Node->ParentComponentOrVariableName ) )
            {
                return Found;
            }
        }

        return nullptr;
    }

    const USceneComponent * FindParentRoot( const UBlueprint * Blueprint )
    {
        if ( const AActor * NativeCDO = FindNativeCDO( Blueprint ) )
        {
            if ( const USceneComponent * NativeRoot = NativeCDO->GetRootComponent() )
            {
                return NativeRoot;
            }
        }

        const TArray< const USimpleConstructionScript * > Scripts = FindParentScs( Blueprint );

        for ( int32 Idx = Scripts.Num() - 1; Idx >= 0; --Idx )
        {
            for ( const USCS_Node * RootNode : Scripts[ Idx ]->GetRootNodes() )
            {
                if ( const USceneComponent * Root = RootNode ? Cast< USceneComponent >( RootNode->ComponentTemplate ) : nullptr )
                {
                    return Root;
                }
            }
        }

        return nullptr;
    }

    const USceneComponent * FindParentTpl( const UBlueprint * Blueprint, const USimpleConstructionScript * Script, USCS_Node * Node )
    {
        if ( !Script )
        {
            return nullptr;
        }

        if ( const USCS_Node * ParentNode = Script->FindParentNode( Node ) )
        {
            return Cast< USceneComponent >( ParentNode->ComponentTemplate );
        }

        if ( const USceneComponent * Named = FindInheritedTpl( Blueprint, Node ) )
        {
            return Named;
        }

        return FindParentRoot( Blueprint );
    }

    bool ImplementsProvider( const UBlueprint * Blueprint )
    {
        if ( !Blueprint )
        {
            return false;
        }

        const UClass * ProviderClass = UPromptBeckonPointProvider::StaticClass();
        if ( Blueprint->ParentClass && Blueprint->ParentClass->ImplementsInterface( ProviderClass ) )
        {
            return true;
        }

        return Blueprint->ImplementedInterfaces.ContainsByPredicate( [ ProviderClass ]( const FBPInterfaceDescription & Desc )
        {
            return Desc.Interface && Desc.Interface->IsChildOf( ProviderClass );
        } );
    }

    void AddErr( FName PointName, EPromptBeckonShapeProblem Problem, bool bFromNative, TArray< FPromptBeckonPointProblem > & OutErrs )
    {
        if ( Problem != EPromptBeckonShapeProblem::None )
        {
            OutErrs.Add( { PointName, Problem, bFromNative } );
        }
    }
} // namespace

void PromptBeckonEditor::FindPointErrs( const UBlueprint * Blueprint, TArray< FPromptBeckonPointProblem > & OutErrs )
{
    const USimpleConstructionScript * Script = Blueprint ? Blueprint->SimpleConstructionScript.Get() : nullptr;
    if ( !Script )
    {
        return;
    }

    const bool bHasProvider = ImplementsProvider( Blueprint );

    for ( USCS_Node * Node : Script->GetAllNodes() )
    {
        if ( Node && Cast< UPromptBeckonPointComponent >( Node->ComponentTemplate ) )
        {
            AddErr( Node->GetVariableName(), UPromptBeckonPointComponent::FindShapeErr( FindParentTpl( Blueprint, Script, Node ) ), false, OutErrs );
            AddErr( Node->GetVariableName(), bHasProvider ? EPromptBeckonShapeProblem::None : EPromptBeckonShapeProblem::NoProvider, false, OutErrs );
        }
    }

    if ( const AActor * NativeCDO = FindNativeCDO( Blueprint ) )
    {
        NativeCDO->ForEachComponent< UPromptBeckonPointComponent >( false, [ &OutErrs, bHasProvider ]( const UPromptBeckonPointComponent * Point )
        {
            AddErr( Point->GetFName(), UPromptBeckonPointComponent::FindShapeErr( Point->GetAttachParent() ), true, OutErrs );
            AddErr( Point->GetFName(), bHasProvider ? EPromptBeckonShapeProblem::None : EPromptBeckonShapeProblem::NoProvider, false, OutErrs );
        } );
    }
}
