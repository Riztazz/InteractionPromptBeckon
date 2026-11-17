#pragma once

#include "Components/ActorComponent.h"
#include "Containers/UnrealString.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "Interaction/InteractionSettings.h"
#include "UObject/Object.h"

namespace PromptBeckon
{
    inline FString OwnerNameOf( const UActorComponent * Comp )
    {
        const AActor * Owner = Comp ? Comp->GetOwner() : nullptr;

        return Owner ? Owner->GetName() : FString( TEXT( "None" ) );
    }

    inline bool HasLeftPlay( const UActorComponent * Comp, bool bWasPlaying )
    {
        return !IsValid( Comp ) || ( bWasPlaying && !Comp->HasBegunPlay() );
    }

    inline ECollisionChannel DetectChannel()
    {
        return GetDefault< UPromptBeckonInteractionSettings >()->DetectChannel;
    }

    inline ECollisionChannel OccludeChannel()
    {
        return GetDefault< UPromptBeckonInteractionSettings >()->OccludeChannel;
    }
} // namespace PromptBeckon
