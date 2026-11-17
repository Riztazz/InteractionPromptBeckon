#include "Interaction/PointComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Interaction/Drivers/InteractionDriver.h"
#include "Interaction/InteractionGate.h"
#include "Interaction/InteractionTags.h"
#include "Interaction/InteractorComponent.h"
#include "Interaction/PBDefinition.h"
#include "Interaction/PointProvider.h"
#include "Logging/StructuredLog.h"
#include "PBCommon.h"
#include "PBEffects.h"
#include "TimerManager.h"
#include "UObject/Class.h"
#include "UObject/WeakObjectPtr.h"

#if WITH_EDITOR
#include "Logging/MessageLog.h"
#include "Logging/TokenizedMessage.h"
#include "Misc/UObjectToken.h"
#endif

namespace
{
#if WITH_EDITOR
    void AddMapCheckMessage( FMessageLog & MapCheck, const AActor * Owner, bool bError, const FText & Text )
    {
        const TSharedRef< FTokenizedMessage > Message = bError ? MapCheck.Error() : MapCheck.Warning();
        Message->AddToken( FUObjectToken::Create( Owner ) );
        Message->AddToken( FTextToken::Create( Text ) );
    }
#endif

    bool HasDriver( const UPromptBeckonDefinition * Definition )
    {
        return Definition && Definition->GetDriver();
    }

    const FPromptBeckonDriver * FindDriver( const FPromptBeckonInteractionSession & Session )
    {
        return Session.Definition ? Session.Definition->GetDriver() : nullptr;
    }
} // namespace

UPromptBeckonPointComponent::UPromptBeckonPointComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

#if WITH_EDITORONLY_DATA
    bVisualizeComponent = true;
#endif
}

void UPromptBeckonPointComponent::BeginPlay()
{
    Super::BeginPlay();

    if ( !Definition )
    {
        UE_LOGFMT( LogPromptBeckon, Warning, "Point '{Point}' on '{Owner}' has no Definition. It cannot be used.", ( "Point", GetName() ), ( "Owner", PromptBeckon::OwnerNameOf( this ) ) );
    }
    else if ( !Definition->GetDriver() )
    {
        UE_LOGFMT( LogPromptBeckon, Warning, "Point '{Point}' on '{Owner}': definition '{Definition}' has no Driver. It cannot be used.", ( "Definition", Definition->GetName() ), ( "Point", GetName() ), ( "Owner", PromptBeckon::OwnerNameOf( this ) ) );
    }

    const EPromptBeckonShapeProblem ShapeErr = FindShapeErr( GetAttachParent() );
    if ( ShapeErr == EPromptBeckonShapeProblem::None )
    {
        return;
    }

    if ( IsShapeErr( ShapeErr ) )
    {
        UE_LOGFMT( LogPromptBeckon, Error, "Point '{Point}' on '{Owner}': {Problem}", ( "Point", GetName() ), ( "Owner", PromptBeckon::OwnerNameOf( this ) ), ( "Problem", GetShapeErrText( ShapeErr ).ToString() ) );
    }
    else
    {
        UE_LOGFMT( LogPromptBeckon, Warning, "Point '{Point}' on '{Owner}': {Problem}", ( "Point", GetName() ), ( "Owner", PromptBeckon::OwnerNameOf( this ) ), ( "Problem", GetShapeErrText( ShapeErr ).ToString() ) );
    }
}

void UPromptBeckonPointComponent::EndPlay( EEndPlayReason::Type EndPlayReason )
{
    {
        FPromptBeckonEffectScope Scope( this );

        if ( const UWorld * World = GetWorld() )
        {
            World->GetTimerManager().ClearTimer( CooldownTimer );
        }

        EndSession( PromptBeckon::Tags::ResultAbortedTargetDestroyed, false, State, Scope );
        State = EPromptBeckonPointState::Available;
    }

    Super::EndPlay( EndPlayReason );
}

UPrimitiveComponent * UPromptBeckonPointComponent::GetShape() const
{
    return Cast< UPrimitiveComponent >( GetAttachParent() );
}

void UPromptBeckonPointComponent::ConfigShape( UPrimitiveComponent * Shape )
{
    if ( !Shape )
    {
        UE_LOGFMT( LogPromptBeckon, Warning, "ConfigShape got no shape." );

        return;
    }

    Shape->SetCollisionEnabled( ECollisionEnabled::QueryOnly );
    Shape->SetCollisionResponseToAllChannels( ECR_Ignore );
    Shape->SetCollisionResponseToChannel( PromptBeckon::DetectChannel(), ECR_Overlap );
    Shape->SetGenerateOverlapEvents( false );
    Shape->SetCanEverAffectNavigation( false );
}

EPromptBeckonShapeProblem UPromptBeckonPointComponent::FindShapeErr( const USceneComponent * AttachParent )
{
    const UPrimitiveComponent * Shape = Cast< UPrimitiveComponent >( AttachParent );
    if ( !Shape )
    {
        return EPromptBeckonShapeProblem::NoShape;
    }

    if ( !CollisionEnabledHasQuery( Shape->GetCollisionEnabled() ) )
    {
        return EPromptBeckonShapeProblem::QueryCollisionOff;
    }

    if ( Shape->GetCollisionResponseToChannel( PromptBeckon::DetectChannel() ) == ECR_Ignore )
    {
        return EPromptBeckonShapeProblem::IgnoresDetectionChannel;
    }

    return EPromptBeckonShapeProblem::None;
}

FText UPromptBeckonPointComponent::GetShapeErrText( EPromptBeckonShapeProblem Problem )
{
    switch ( Problem )
    {
        case EPromptBeckonShapeProblem::NoShape:
            return NSLOCTEXT( "PromptBeckon", "ShapeProblemNoShape", "The point is not attached to a primitive. Attach it to a sphere or box, then call Config Shape." );
        case EPromptBeckonShapeProblem::QueryCollisionOff:
            return NSLOCTEXT( "PromptBeckon", "ShapeProblemQueryCollisionOff", "The shape has query collision off. Set Collision Enabled to Query Only." );
        case EPromptBeckonShapeProblem::IgnoresDetectionChannel:
            return NSLOCTEXT( "PromptBeckon", "ShapeProblemIgnoresDetectionChannel", "The shape ignores the Detect Channel. Set it to Overlap, or call Config Shape." );
        case EPromptBeckonShapeProblem::NoProvider:
            return NSLOCTEXT( "PromptBeckon", "ShapeProblemNoProvider", "The actor does not implement the Prompt Beckon Point Provider interface. Add it and return this point from Get Points." );
        case EPromptBeckonShapeProblem::None:
            break;
    }

    return FText::GetEmpty();
}

bool UPromptBeckonPointComponent::IsShapeErr( EPromptBeckonShapeProblem Problem )
{
    return Problem == EPromptBeckonShapeProblem::NoShape || Problem == EPromptBeckonShapeProblem::NoProvider;
}

#if WITH_EDITOR
void UPromptBeckonPointComponent::CheckForErrors()
{
    Super::CheckForErrors();

    FMessageLog MapCheck( "MapCheck" );
    const AActor * Owner = GetOwner();

    const EPromptBeckonShapeProblem ShapeErr = FindShapeErr( GetAttachParent() );
    if ( ShapeErr != EPromptBeckonShapeProblem::None )
    {
        AddMapCheckMessage( MapCheck, Owner, IsShapeErr( ShapeErr ), GetShapeErrText( ShapeErr ) );
    }

    if ( Owner && !Owner->Implements< UPromptBeckonPointProvider >() )
    {
        AddMapCheckMessage( MapCheck, Owner, IsShapeErr( EPromptBeckonShapeProblem::NoProvider ), GetShapeErrText( EPromptBeckonShapeProblem::NoProvider ) );
    }
}
#endif

void UPromptBeckonPointComponent::SetDefinition( const UPromptBeckonDefinition * NewDefinition )
{
    Definition = NewDefinition;
}

const UPromptBeckonDefinition * UPromptBeckonPointComponent::GetDefinition() const
{
    return Definition;
}

void UPromptBeckonPointComponent::SetAvail( EPromptBeckonAvailability NewAvail, const FText & Reason )
{
    FPromptBeckonEffectScope Scope( this );

    UnavailableReason = Reason;
    if ( Availability == NewAvail )
    {
        return;
    }

    const EPromptBeckonAvailability OldAvail = Availability;
    Availability                             = NewAvail;

    Scope.AddFact( this, [ OldAvail, NewAvail ]( UPromptBeckonPointComponent * Point )
    {
        Point->OnAvailChanged.Broadcast( Point, OldAvail, NewAvail );
    } );

    if ( NewAvail != EPromptBeckonAvailability::Available )
    {
        EndSession( PromptBeckon::Tags::ResultAbortedUnavailable, false, EPromptBeckonPointState::Available, Scope );
    }
}

EPromptBeckonAvailability UPromptBeckonPointComponent::GetAvail() const
{
    return Availability;
}

const FText & UPromptBeckonPointComponent::GetUnavailReason() const
{
    return UnavailableReason;
}

void UPromptBeckonPointComponent::ResetAvail()
{
    FPromptBeckonEffectScope Scope( this );

    if ( const UWorld * World = GetWorld() )
    {
        World->GetTimerManager().ClearTimer( CooldownTimer );
    }

    EndSession( PromptBeckon::Tags::ResultAbortedReset, false, EPromptBeckonPointState::Available, Scope );
    SetState( EPromptBeckonPointState::Available, Scope );
}

EPromptBeckonPointState UPromptBeckonPointComponent::GetState() const
{
    return State;
}

bool UPromptBeckonPointComponent::CanInteract() const
{
    return State == EPromptBeckonPointState::Available && Availability == EPromptBeckonAvailability::Available && HasDriver( Definition );
}

EPromptBeckonPromptVisibility UPromptBeckonPointComponent::GetVis() const
{
    const bool bHideOnCooldown = Definition ? Definition->bHideOnCooldown : true;

    return CalcVis( State, Availability, HasDriver( Definition ), bHideOnCooldown );
}

EPromptBeckonPromptVisibility UPromptBeckonPointComponent::CalcVis( EPromptBeckonPointState InState, EPromptBeckonAvailability InAvail, bool bDefValid, bool bHideOnCooldown )
{
    if ( !bDefValid || InAvail == EPromptBeckonAvailability::Hidden )
    {
        return EPromptBeckonPromptVisibility::Hidden;
    }

    if ( InState == EPromptBeckonPointState::InProgress || ( InState == EPromptBeckonPointState::Available && InAvail == EPromptBeckonAvailability::Available ) )
    {
        return EPromptBeckonPromptVisibility::Enabled;
    }

    if ( bHideOnCooldown && InAvail != EPromptBeckonAvailability::Refused )
    {
        return EPromptBeckonPromptVisibility::Hidden;
    }

    return EPromptBeckonPromptVisibility::Disabled;
}

bool UPromptBeckonPointComponent::Press( UPromptBeckonInteractorComponent * Interactor )
{
    FPromptBeckonEffectScope Scope( this );

    if ( !Interactor )
    {
        UE_LOGFMT( LogPromptBeckon, Verbose, "Point '{Point}': press refused, no interactor.", GetName() );

        return false;
    }

    if ( IsHeldBy( Interactor ) )
    {
        return ApplyPress( Interactor, Scope );
    }

    if ( Session.IsSet() && !Session.Holder.IsValid() )
    {
        EndSession( PromptBeckon::Tags::ResultAbortedInteractorDestroyed, false, EPromptBeckonPointState::Available, Scope );
    }

    if ( Session.IsSet() )
    {
        UE_LOGFMT( LogPromptBeckon, Verbose, "Point '{Point}': press by '{Interactor}' refused, '{Holder}' holds it.", ( "Point", GetName() ), ( "Interactor", Interactor->GetName() ), ( "Holder", GetNameSafe( Session.Holder.Get() ) ) );

        return false;
    }

    if ( !CanInteract() )
    {
        UE_LOGFMT( LogPromptBeckon, Verbose, "Point '{Point}': press refused. State {State}, availability {Avail}, has driver {Driver}.", ( "Point", GetName() ), ( "State", UEnum::GetValueAsString( State ) ), ( "Avail", UEnum::GetValueAsString( Availability ) ), ( "Driver", HasDriver( Definition ) ) );

        return false;
    }

    const int32 Generation = Scope.GetGen();
    if ( Generation >= FPromptBeckonEffectScope::MaxGen )
    {
        UE_LOGFMT( LogPromptBeckon, Error, "Point '{Point}' on '{PointOwner}': press by '{Interactor}' on '{Owner}' refused, {Generation} steps deep. A handler presses again on every finish.", ( "Interactor", Interactor->GetName() ), ( "Owner", PromptBeckon::OwnerNameOf( Interactor ) ), ( "Point", GetName() ), ( "PointOwner", PromptBeckon::OwnerNameOf( this ) ), ( "Generation", Generation ) );

        return false;
    }

    const TWeakObjectPtr< UPromptBeckonPointComponent > Self( this );
    const TWeakObjectPtr< UPromptBeckonInteractorComponent > Pressing( Interactor );
    const bool bWasPlaying = HasBegunPlay();

    if ( !AllowsUse( Interactor ) )
    {
        return false;
    }

    UPromptBeckonPointComponent * Point        = Self.Get();
    UPromptBeckonInteractorComponent * Claimer = Pressing.Get();
    if ( PromptBeckon::HasLeftPlay( Point, bWasPlaying ) || !Claimer || !Point->CanInteract() || Point->Session.IsSet() || !Claimer->CanClaim( Point ) )
    {
        UE_LOGFMT( LogPromptBeckon, Verbose, "Point '{Point}': press refused, a gate changed the point or the interactor.", GetNameSafe( Point ) );

        return false;
    }

    Session.Id         = FPromptBeckonInteractionSessionId::Next();
    Session.Holder     = Claimer;
    Session.Definition = Definition;
    Session.Run        = FPromptBeckonDriverRun();
    SetState( EPromptBeckonPointState::InProgress, Scope );

    Scope.AddFinal( Claimer, [ Self ]( UPromptBeckonInteractorComponent * Told )
    {
        Told->OnInteractStart.Broadcast( Self.Get() );
    } );

    return ApplyPress( Claimer, Scope );
}

void UPromptBeckonPointComponent::Release( const UPromptBeckonInteractorComponent * Interactor )
{
    FPromptBeckonEffectScope Scope( this );

    if ( !AcceptsHolder( Interactor, TEXT( "release" ) ) )
    {
        return;
    }

    if ( const FPromptBeckonDriver * Driver = ResolveDriver( Scope ) )
    {
        ApplyStep( Driver->Release( Session.Run ), Scope );
    }
}

void UPromptBeckonPointComponent::Advance( const UPromptBeckonInteractorComponent * Interactor, float DeltaSeconds )
{
    FPromptBeckonEffectScope Scope( this );

    if ( !AcceptsHolder( Interactor, TEXT( "advance" ) ) )
    {
        return;
    }

    if ( const FPromptBeckonDriver * Driver = ResolveDriver( Scope ) )
    {
        ApplyStep( Driver->Advance( Session.Run, DeltaSeconds ), Scope );
    }
}

void UPromptBeckonPointComponent::Cancel( const UPromptBeckonInteractorComponent * Interactor, FGameplayTag Result )
{
    FPromptBeckonEffectScope Scope( this );

    if ( !AcceptsHolder( Interactor, TEXT( "cancel" ) ) )
    {
        return;
    }

    EndSession( Result, false, EPromptBeckonPointState::Available, Scope );
}

bool UPromptBeckonPointComponent::IsHeldBy( const UPromptBeckonInteractorComponent * Interactor ) const
{
    return Session.IsSet() && Interactor && Session.Holder.Get() == Interactor;
}

float UPromptBeckonPointComponent::GetProgress() const
{
    const FPromptBeckonDriver * Driver = FindDriver( Session );

    return Session.IsSet() && Driver ? Driver->GetProgress( Session.Run ) : 0.f;
}

bool UPromptBeckonPointComponent::AllowsUse( const UPromptBeckonInteractorComponent * Interactor ) const
{
    if ( !Interactor )
    {
        UE_LOGFMT( LogPromptBeckon, Verbose, "Point '{Point}': press refused, no interactor.", GetName() );

        return false;
    }

    AActor * Owner = GetOwner();
    if ( !Owner )
    {
        return true;
    }

    TArray< TWeakObjectPtr< UObject > > Gates;
    if ( Owner->Implements< UPromptBeckonInteractionGate >() )
    {
        Gates.Add( Owner );
    }

    for ( UActorComponent * Comp : Owner->GetComponents() )
    {
        if ( Comp->Implements< UPromptBeckonInteractionGate >() )
        {
            Gates.Add( Comp );
        }
    }

    const TWeakObjectPtr< const UPromptBeckonPointComponent > Self( this );
    const TWeakObjectPtr< const UPromptBeckonInteractorComponent > Asking( Interactor );

    for ( const TWeakObjectPtr< UObject > & Gate : Gates )
    {
        const UObject * Live = Gate.Get();
        if ( !Live )
        {
            continue;
        }

        const FString GateName = Live->GetName();
        const bool bAllowed    = IPromptBeckonInteractionGate::Execute_CanBegin( Live, Self.Get(), Asking.Get() );

        const UPromptBeckonPointComponent * Point = Self.Get();
        if ( !Point || !Asking.IsValid() )
        {
            UE_LOGFMT( LogPromptBeckon, Verbose, "Press refused: gate '{Gate}' destroyed the point or the interactor.", GateName );

            return false;
        }

        if ( !bAllowed )
        {
            UE_LOGFMT( LogPromptBeckon, Verbose, "Point '{Point}': press refused by gate '{Gate}'.", ( "Point", Point->GetName() ), ( "Gate", GateName ) );

            return false;
        }
    }

    return true;
}

FPromptBeckonInteractionSessionId UPromptBeckonPointComponent::GetSessionId() const
{
    return Session.Id;
}

void UPromptBeckonPointComponent::SetState( EPromptBeckonPointState NewState, FPromptBeckonEffectScope & Scope )
{
    if ( State == NewState )
    {
        return;
    }

    const EPromptBeckonPointState OldState = State;

    State = NewState;

    AddState( OldState, NewState, Scope );
}

void UPromptBeckonPointComponent::AddState( EPromptBeckonPointState OldState, EPromptBeckonPointState NewState, FPromptBeckonEffectScope & Scope )
{
    Scope.AddFact( this, [ OldState, NewState ]( UPromptBeckonPointComponent * Point )
    {
        Point->OnStateChanged.Broadcast( Point, OldState, NewState );
    } );
}

void UPromptBeckonPointComponent::ApplyStep( const FPromptBeckonDriverStep & Step, FPromptBeckonEffectScope & Scope )
{
    switch ( Step.Step )
    {
        case EPromptBeckonInteractionStep::Continue:
            break;
        case EPromptBeckonInteractionStep::Complete:
            EndSession( PromptBeckon::Tags::ResultCompleted, true, GetCooldown() > 0.f ? EPromptBeckonPointState::Cooldown : EPromptBeckonPointState::Available, Scope );
            break;
        case EPromptBeckonInteractionStep::Abort:
            EndSession( Step.AbortReason, false, EPromptBeckonPointState::Available, Scope );
            break;
    }
}

void UPromptBeckonPointComponent::EndSession( FGameplayTag Result, bool bCompleted, EPromptBeckonPointState NewState, FPromptBeckonEffectScope & Scope )
{
    if ( !Session.IsSet() )
    {
        return;
    }

    const EPromptBeckonPointState OldState                          = State;
    const TWeakObjectPtr< UPromptBeckonInteractorComponent > Holder = Session.Holder;
    const float CooldownSeconds                                     = bCompleted ? GetCooldown() : 0.f;

    Session.Reset();
    State = NewState;

    if ( CooldownSeconds > 0.f )
    {
        GetWorld()->GetTimerManager().SetTimer( CooldownTimer, this, &UPromptBeckonPointComponent::OnCooldownDone, CooldownSeconds, false );
    }

    if ( bCompleted )
    {
        Scope.AddFact( this, [ Holder ]( UPromptBeckonPointComponent * Point )
        {
            Point->OnInteracted.Broadcast( Point, Holder.Get() );
        } );
    }

    if ( OldState != NewState )
    {
        AddState( OldState, NewState, Scope );
    }

    Scope.AddFact( this, [ Result ]( UPromptBeckonPointComponent * Point )
    {
        Point->OnInteractEnd.Broadcast( Point, Result );
    } );

    const TWeakObjectPtr< UPromptBeckonPointComponent > Self( this );
    Scope.AddFinal( Holder.Get( true ), [ Self, Result ]( UPromptBeckonInteractorComponent * Told )
    {
        Told->OnInteractEnd.Broadcast( Self.Get(), Result );
    } );
}

bool UPromptBeckonPointComponent::ApplyPress( const UPromptBeckonInteractorComponent * Interactor, FPromptBeckonEffectScope & Scope )
{
    const FPromptBeckonDriver * Driver = ResolveDriver( Scope );
    if ( !Driver )
    {
        return false;
    }

    const FPromptBeckonDriverStep Step = Driver->Press( Session.Run );
    ApplyStep( Step, Scope );

    return Step.Step == EPromptBeckonInteractionStep::Complete || IsHeldBy( Interactor );
}

const FPromptBeckonDriver * UPromptBeckonPointComponent::ResolveDriver( FPromptBeckonEffectScope & Scope )
{
    if ( !Session.IsSet() )
    {
        return nullptr;
    }

    if ( const FPromptBeckonDriver * Driver = FindDriver( Session ) )
    {
        return Driver;
    }

    UE_LOGFMT( LogPromptBeckon, Warning, "Point '{Point}' on '{Owner}': Driver of '{Definition}' was cleared mid-press. The press ends.", ( "Point", GetName() ), ( "Owner", PromptBeckon::OwnerNameOf( this ) ), ( "Definition", GetNameSafe( Session.Definition.Get() ) ) );

    EndSession( PromptBeckon::Tags::ResultAbortedReset, false, EPromptBeckonPointState::Available, Scope );

    return nullptr;
}

float UPromptBeckonPointComponent::GetCooldown() const
{
    const float CooldownSeconds = Session.IsSet() && Session.Definition ? Session.Definition->CooldownSeconds : 0.f;

    return CooldownSeconds > 0.f && GetWorld() ? CooldownSeconds : 0.f;
}

bool UPromptBeckonPointComponent::AcceptsHolder( const UPromptBeckonInteractorComponent * Interactor, const TCHAR * Call ) const
{
    if ( IsHeldBy( Interactor ) )
    {
        return true;
    }

    UE_LOGFMT( LogPromptBeckon, Verbose, "Point '{Point}': {Call} by '{Interactor}' refused, it does not hold the point.", ( "Point", GetName() ), ( "Call", Call ), ( "Interactor", GetNameSafe( Interactor ) ) );

    return false;
}

void UPromptBeckonPointComponent::OnCooldownDone()
{
    FPromptBeckonEffectScope Scope( this );

    if ( !ensureMsgf( State == EPromptBeckonPointState::Cooldown, TEXT( "Point '%s' left a cooldown it was not in." ), *GetName() ) )
    {
        return;
    }

    SetState( EPromptBeckonPointState::Available, Scope );
}
