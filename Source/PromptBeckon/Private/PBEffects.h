#pragma once

#include "Components/ActorComponent.h"
#include "Containers/Array.h"
#include "Interaction/PBTypes.h"
#include "Misc/CoreMiscDefines.h"
#include "Templates/Function.h"
#include "Templates/Identity.h"
#include "UObject/WeakObjectPtr.h"

#include <type_traits>

class UObject;
class UPromptBeckonEffectSubsystem;
class UPromptBeckonPointComponent;

enum class EPromptBeckonEffectRule : uint8
{
    Fact,
    FinalFact,
    Level,
    Command
};

struct FPromptBeckonEffect
{
    TWeakObjectPtr< UObject > Subject;
    EPromptBeckonEffectRule Rule = EPromptBeckonEffectRule::Fact;
    bool bWasPlaying             = false;
    int32 Generation             = 0;
    FPromptBeckonInteractionSessionId SessionId;
    TFunction< void( UObject * ) > Deliver;
};

/**
 * Purpose: holds back every event the plugin raises until the call that raised it has finished, then tells them in order.
 * Without it, a game handler that calls back in (SetAvail inside OnInteracted) would run in the middle of
 * the first call and see half-changed state.
 *
 * Usage:
 * 1. Open one on the stack at the start of each public entry point, passing the component as WorldContext.
 * 2. Queue each event with AddFact, AddFinal, AddLvl or AddCmd instead of broadcasting it.
 * 3. The events are told when the outermost scope of that world ends.
 */
class PROMPTBECKON_API FPromptBeckonEffectScope
{
public:
    /** Joins the open scope of WorldContext's world, or owns the queue when none is open or there is no world. */
    explicit FPromptBeckonEffectScope( const UObject * WorldContext );
    ~FPromptBeckonEffectScope();

    template< typename T >
    void AddFact( const T * Subject, TFunction< void( TIdentity_T< T > * ) > Deliver );
    template< typename T >
    void AddFinal( const T * Subject, TFunction< void( TIdentity_T< T > * ) > Deliver );
    template< typename T >
    void AddLvl( const T * Subject, TFunction< void( TIdentity_T< T > * ) > Deliver );
    void AddCmd( const UPromptBeckonPointComponent * Point, FPromptBeckonInteractionSessionId SessionId, TFunction< void( UPromptBeckonPointComponent * ) > Deliver );

    int32 GetGen() const;

    static constexpr int32 MaxGen = 32;

private:
    UE_NONCOPYABLE( FPromptBeckonEffectScope );

    void Add( const UActorComponent * Subject, EPromptBeckonEffectRule Rule, FPromptBeckonInteractionSessionId SessionId, TFunction< void( UObject * ) > && Deliver );

    static UObject * Resolve( const FPromptBeckonEffect & Effect );

    template< typename T >
    static TFunction< void( UObject * ) > Erase( TFunction< void( T * ) > && Deliver )
    {
        static_assert( std::is_base_of_v< UActorComponent, T >, "An effect subject is an actor component: the rules read whether it plays." );

        if ( !Deliver )
        {
            return TFunction< void( UObject * ) >();
        }

        return [ Typed = MoveTemp( Deliver ) ]( UObject * Subject )
        {
            Typed( static_cast< T * >( Subject ) );
        };
    }

    /** The scope that owns the queue: this one, or the outermost open scope of the same world. */
    FPromptBeckonEffectScope * Owner = nullptr;

    /** The subsystem this scope registered with as its world's owner, so it can leave it when it ends. */
    TWeakObjectPtr< UPromptBeckonEffectSubsystem > Subsystem;

    TArray< FPromptBeckonEffect > Effects;
    int32 Gen       = 0;
    bool bCapLogged = false;
};

template< typename T >
void FPromptBeckonEffectScope::AddFact( const T * Subject, TFunction< void( TIdentity_T< T > * ) > Deliver )
{
    Add( Subject, EPromptBeckonEffectRule::Fact, FPromptBeckonInteractionSessionId(), Erase< T >( MoveTemp( Deliver ) ) );
}

template< typename T >
void FPromptBeckonEffectScope::AddFinal( const T * Subject, TFunction< void( TIdentity_T< T > * ) > Deliver )
{
    Add( Subject, EPromptBeckonEffectRule::FinalFact, FPromptBeckonInteractionSessionId(), Erase< T >( MoveTemp( Deliver ) ) );
}

template< typename T >
void FPromptBeckonEffectScope::AddLvl( const T * Subject, TFunction< void( TIdentity_T< T > * ) > Deliver )
{
    Add( Subject, EPromptBeckonEffectRule::Level, FPromptBeckonInteractionSessionId(), Erase< T >( MoveTemp( Deliver ) ) );
}
