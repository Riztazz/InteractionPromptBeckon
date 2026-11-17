#include "PBEffects.h"

#include "CoreGlobals.h"
#include "EffectSubsystem.h"
#include "Engine/World.h"
#include "Interaction/PBTypes.h"
#include "Interaction/PointComponent.h"
#include "Logging/StructuredLog.h"
#include "Misc/AssertionMacros.h"
#include "PBCommon.h"
#include "UObject/Object.h"

FPromptBeckonEffectScope::FPromptBeckonEffectScope( const UObject * WorldContext )
{
    checkf( IsInGameThread(), TEXT( "Prompt Beckon runs on the game thread only." ) );

    const UWorld * World                 = WorldContext ? WorldContext->GetWorld() : nullptr;
    UPromptBeckonEffectSubsystem * Found = World ? World->GetSubsystem< UPromptBeckonEffectSubsystem >() : nullptr;

    if ( Found && Found->OwnerScope )
    {
        Owner = Found->OwnerScope;

        return;
    }

    Owner = this;

    if ( Found )
    {
        Found->OwnerScope = this;
        Subsystem         = Found;
    }
}

FPromptBeckonEffectScope::~FPromptBeckonEffectScope()
{
    if ( Owner != this )
    {
        return;
    }

    for ( int32 Idx = 0; Idx < Effects.Num(); ++Idx )
    {
        FPromptBeckonEffect Effect = MoveTemp( Effects[ Idx ] );

        UObject * Subject = Resolve( Effect );
        if ( !Subject )
        {
            continue;
        }

        if ( !bCapLogged && Effect.Generation >= MaxGen )
        {
            bCapLogged = true;

            UE_LOGFMT( LogPromptBeckon, Warning, "Event loop on '{Subject}': {Generation} steps deep. A handler keeps calling back. Events still go out.", ( "Generation", Effect.Generation ), ( "Subject", GetPathNameSafe( Subject ) ) );
        }

        Gen = Effect.Generation + 1;
        Effect.Deliver( Subject );
    }

    UPromptBeckonEffectSubsystem * Registered = Subsystem.Get();
    if ( Registered && Registered->OwnerScope == this )
    {
        Registered->OwnerScope = nullptr;
    }
}

void FPromptBeckonEffectScope::AddCmd( const UPromptBeckonPointComponent * Point, FPromptBeckonInteractionSessionId SessionId, TFunction< void( UPromptBeckonPointComponent * ) > Deliver )
{
    Add( Point, EPromptBeckonEffectRule::Command, SessionId, Erase< UPromptBeckonPointComponent >( MoveTemp( Deliver ) ) );
}

int32 FPromptBeckonEffectScope::GetGen() const
{
    return Owner->Gen;
}

void FPromptBeckonEffectScope::Add( const UActorComponent * Subject, EPromptBeckonEffectRule Rule, FPromptBeckonInteractionSessionId SessionId, TFunction< void( UObject * ) > && Deliver )
{
    if ( !Subject || ( Rule == EPromptBeckonEffectRule::Command && !SessionId.IsSet() ) )
    {
        return;
    }

    FPromptBeckonEffect & Effect = Owner->Effects.AddDefaulted_GetRef();

    Effect.Subject     = const_cast< UActorComponent * >( Subject );
    Effect.Rule        = Rule;
    Effect.bWasPlaying = Subject->HasBegunPlay();
    Effect.Generation  = Owner->Gen;
    Effect.SessionId   = SessionId;
    Effect.Deliver     = MoveTemp( Deliver );
}

UObject * FPromptBeckonEffectScope::Resolve( const FPromptBeckonEffect & Effect )
{
    switch ( Effect.Rule )
    {
        case EPromptBeckonEffectRule::Fact:
        {
            const UActorComponent * Comp = Cast< UActorComponent >( Effect.Subject.Get() );

            return PromptBeckon::HasLeftPlay( Comp, Effect.bWasPlaying ) ? nullptr : Effect.Subject.Get();
        }
        case EPromptBeckonEffectRule::FinalFact:
            return Effect.Subject.Get( true );
        case EPromptBeckonEffectRule::Level:
            return Effect.Subject.Get();
        case EPromptBeckonEffectRule::Command:
        {
            UPromptBeckonPointComponent * Point = Cast< UPromptBeckonPointComponent >( Effect.Subject.Get() );

            return Point && Point->GetSessionId() == Effect.SessionId ? Point : nullptr;
        }
    }

    return nullptr;
}
