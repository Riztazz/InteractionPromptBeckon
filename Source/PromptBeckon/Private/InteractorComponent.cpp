#include "Interaction/InteractorComponent.h"

#include "Algo/Find.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/InteractionTags.h"
#include "Interaction/PBDefinition.h"
#include "Interaction/PointComponent.h"
#include "Interaction/PointProvider.h"
#include "Interaction/Presenters/Presenter.h"
#include "Interaction/Presenters/ScreenPresenter.h"
#include "Interaction/Presenters/WorldPresenter.h"
#include "Interaction/Scorers/FocusScorer.h"
#include "Interaction/Scorers/ReticleScorer.h"
#include "Logging/StructuredLog.h"
#include "Math/Vector2D.h"
#include "Misc/AssertionMacros.h"
#include "Misc/Optional.h"
#include "PBCommon.h"
#include "PBEffects.h"
#include "Templates/UnrealTemplate.h"
#include "UObject/NameTypes.h"

#if ENABLE_VISUAL_LOG
#include "VisualLogger/VisualLogger.h"
#include "VisualLogger/VisualLoggerTypes.h"
#endif

namespace
{
    constexpr EPromptBeckonPresenterKind PresenterKinds[] = { EPromptBeckonPresenterKind::Screen, EPromptBeckonPresenterKind::World };

    EPromptBeckonPresenterKind PresenterKindOf( const UPromptBeckonDefinition * Definition )
    {
        return Definition ? Definition->Presenter : EPromptBeckonPresenterKind::Screen;
    }
} // namespace

UPromptBeckonInteractorComponent::UPromptBeckonInteractorComponent()
{
    PrimaryComponentTick.bCanEverTick          = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.TickGroup             = TG_PostUpdateWork;

    Scorer.InitializeAs< FPromptBeckonReticleScorer >();
    ScreenPresenter = CreateDefaultSubobject< UPromptBeckonScreenPresenter >( TEXT( "ScreenPresenter" ) );
    WorldPresenter  = CreateDefaultSubobject< UPromptBeckonWorldPresenter >( TEXT( "WorldPresenter" ) );
}

void UPromptBeckonInteractorComponent::BeginPlay()
{
    Super::BeginPlay();

#if ENABLE_VISUAL_LOG
    REDIRECT_OBJECT_TO_VLOG( this, GetOwner() );
#endif

    if ( !Cast< APawn >( GetOwner() ) )
    {
        UE_LOGFMT( LogPromptBeckon, Warning, "Interactor '{Interactor}' on '{Owner}' is not on a pawn. Put it on a player pawn.", ( "Interactor", GetName() ), ( "Owner", PromptBeckon::OwnerNameOf( this ) ) );
    }

    if ( !Scorer.IsValid() )
    {
        UE_LOGFMT( LogPromptBeckon, Warning, "Interactor '{Interactor}' on '{Owner}' has no Scorer. Nothing gets focus.", ( "Interactor", GetName() ), ( "Owner", PromptBeckon::OwnerNameOf( this ) ) );
    }

    for ( const EPromptBeckonPresenterKind Kind : PresenterKinds )
    {
        if ( GetPresenter( Kind ) )
        {
            continue;
        }

        const FName Property = Kind == EPromptBeckonPresenterKind::World ? GET_MEMBER_NAME_CHECKED( UPromptBeckonInteractorComponent, WorldPresenter ) : GET_MEMBER_NAME_CHECKED( UPromptBeckonInteractorComponent, ScreenPresenter );

        UE_LOGFMT( LogPromptBeckon, Warning, "Interactor '{Interactor}' on '{Owner}' has no {Property}. Those prompts do not show.", ( "Interactor", GetName() ), ( "Owner", PromptBeckon::OwnerNameOf( this ) ), ( "Property", Property.ToString() ) );
    }
}

void UPromptBeckonInteractorComponent::EndPlay( EEndPlayReason::Type EndPlayReason )
{
    {
        FPromptBeckonEffectScope Scope( this );

        if ( UPromptBeckonPointComponent * Held = GetHeld() )
        {
            Held->Cancel( this, PromptBeckon::Tags::ResultAbortedInteractorDestroyed );
        }

        WantedViewer.Reset();
        PendingPresent.Reset();
        NumDetected = 0;
        SetFocus( nullptr, Scope );

        UPromptBeckonPresenter * const EndedScreen = ScreenViewer.IsExplicitlyNull() ? nullptr : ScreenPresenter.Get();
        UPromptBeckonPresenter * const EndedWorld  = WorldViewer.IsExplicitlyNull() ? nullptr : WorldPresenter.Get();
        ScreenViewer.Reset();
        WorldViewer.Reset();

        if ( EndedScreen )
        {
            EndedScreen->End();
        }

        if ( EndedWorld )
        {
            EndedWorld->End();
        }
    }

    Super::EndPlay( EndPlayReason );
}

void UPromptBeckonInteractorComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction )
{
    Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

    FPromptBeckonEffectScope Scope( this );

    if ( bSuspended )
    {
        return;
    }

    if ( !FindViewer() )
    {
        LoseViewer( Scope );

        return;
    }

    const TWeakObjectPtr< UPromptBeckonInteractorComponent > Gathering( this );
    TArray< UPromptBeckonPointComponent * > Points;
    GatherPoints( Points );

    if ( !Gathering.IsValid() || !HasBegunPlay() || bSuspended )
    {
        return;
    }

    APlayerController * LocalViewer = FindViewer();
    if ( !LocalViewer )
    {
        LoseViewer( Scope );

        return;
    }

    WantedViewer = LocalViewer;

    FPromptBeckonFocusView View;
    LocalViewer->GetPlayerViewPoint( View.Location, View.Rotation );
    View.DetectRadius = DetectRadius;

    const FPromptBeckonFocusScorer * Scoring = Scorer.GetPtr();
    UPromptBeckonPointComponent * Held       = GetHeld();

    TArray< UPromptBeckonPointComponent * > Kept;
    TArray< TOptional< float > > Costs;
    TArray< FPromptBeckonPromptView > Prompts;
    Kept.Reserve( Points.Num() );
    Costs.Reserve( Points.Num() );
    Prompts.Reserve( Points.Num() );

    for ( UPromptBeckonPointComponent * Point : Points )
    {
        const EPromptBeckonPromptVisibility Visibility = Point->GetVis();
        if ( Visibility == EPromptBeckonPromptVisibility::Hidden )
        {
            continue;
        }

        Kept.Add( Point );
        TOptional< float > & Cost = Costs.AddDefaulted_GetRef();

        FPromptBeckonPromptView & Prompt = Prompts.AddDefaulted_GetRef();
        Prompt.Point                     = Point;
        Prompt.Anchor                    = Point->GetComponentTransform();
        Prompt.Definition                = Point->GetDefinition();
        Prompt.Progress                  = Point->IsHeldBy( this ) ? Point->GetProgress() : 0.f;
        Prompt.bEnabled                  = Visibility == EPromptBeckonPromptVisibility::Enabled;

        const bool bCanTakeFocus = Scoring && Point->CanInteract() && ( !bRequireLOS || HasLOS( View.Location, Point ) );
        float Value              = 0.f;
        if ( bCanTakeFocus && Scoring->ScoreCand( View, MakeCand( Point, View, LocalViewer ), Value ) )
        {
            Cost = Value;
        }
    }

    int32 FocusIdx                         = INDEX_NONE;
    UPromptBeckonPointComponent * NewFocus = Held;

    if ( Held )
    {
        FocusIdx = Kept.IndexOfByKey( Held );
    }
    else if ( Scoring )
    {
        const int32 Picked = FPromptBeckonFocusScorer::PickFocus( Costs, Kept.IndexOfByKey( Focused.Get() ), Stickiness, Scoring->MaxCost );
        FocusIdx           = Kept.IsValidIndex( Picked ) ? Picked : INDEX_NONE;
        NewFocus           = Kept.IsValidIndex( FocusIdx ) ? Kept[ FocusIdx ] : nullptr;
    }

    if ( Prompts.IsValidIndex( FocusIdx ) )
    {
        Prompts[ FocusIdx ].bFocused = true;
    }

    NumDetected = Kept.Num();

    FPendingPresent & Pending = PendingPresent.Emplace();
    for ( FPromptBeckonPromptView & Prompt : Prompts )
    {
        Pending.GetShare( PresenterKindOf( Prompt.Definition ) ).Add( MoveTemp( Prompt ) );
    }
    Pending.View         = View;
    Pending.DeltaSeconds = DeltaTime;

    AddSync( Scope );
    SetFocus( NewFocus, Scope );

    if ( !Held )
    {
        return;
    }

    const TWeakObjectPtr< UPromptBeckonInteractorComponent > Self( this );
    if ( FocusIdx == INDEX_NONE )
    {
        Scope.AddCmd( Held, Held->GetSessionId(), [ Self ]( UPromptBeckonPointComponent * Point )
        {
            Point->Cancel( Self.Get(), PromptBeckon::Tags::ResultCancelledOutOfRange );
        } );
    }
    else
    {
        Scope.AddCmd( Held, Held->GetSessionId(), [ Self, DeltaTime ]( UPromptBeckonPointComponent * Point )
        {
            Point->Advance( Self.Get(), DeltaTime );
        } );
    }
}

void UPromptBeckonInteractorComponent::InteractPressed()
{
    FPromptBeckonEffectScope Scope( this );

    if ( bSuspended )
    {
        UE_LOGFMT( LogPromptBeckon, Verbose, "Interactor '{Interactor}': press ignored, suspended.", GetName() );

        return;
    }

    UPromptBeckonPointComponent * Point = Focused.Get();
    if ( !Point )
    {
        UE_LOGFMT( LogPromptBeckon, Verbose, "Interactor '{Interactor}': press ignored, nothing in focus.", GetName() );

        return;
    }

    Point->Press( this );
}

void UPromptBeckonInteractorComponent::InteractReleased()
{
    FPromptBeckonEffectScope Scope( this );

    if ( UPromptBeckonPointComponent * Held = GetHeld() )
    {
        Held->Release( this );
    }
}

void UPromptBeckonInteractorComponent::SetSuspended( bool bInSuspended )
{
    FPromptBeckonEffectScope Scope( this );

    if ( bSuspended == bInSuspended )
    {
        return;
    }

    bSuspended = bInSuspended;
    if ( !bSuspended )
    {
        return;
    }

    NumDetected = 0;

    if ( UPromptBeckonPointComponent * Held = GetHeld() )
    {
        Held->Cancel( this, PromptBeckon::Tags::ResultCancelledSuspended );
    }

    PendingPresent.Reset();
    if ( HasBegun() )
    {
        PendingPresent.Emplace();
    }

    AddSync( Scope );
    SetFocus( nullptr, Scope );
}

bool UPromptBeckonInteractorComponent::IsSuspended() const
{
    return bSuspended;
}

bool UPromptBeckonInteractorComponent::IsInteracting() const
{
    return GetHeld() != nullptr;
}

UPromptBeckonPointComponent * UPromptBeckonInteractorComponent::GetFocusedPoint() const
{
    return Focused.Get();
}

bool UPromptBeckonInteractorComponent::CanClaim( const UPromptBeckonPointComponent * Point ) const
{
    return Point && HasBegunPlay() && !bSuspended && Focused.Get() == Point;
}

UPromptBeckonPointComponent * UPromptBeckonInteractorComponent::GetHeld() const
{
    UPromptBeckonPointComponent * Point = Focused.Get();

    return Point && Point->IsHeldBy( this ) ? Point : nullptr;
}

void UPromptBeckonInteractorComponent::SetFocus( UPromptBeckonPointComponent * NewFocus, FPromptBeckonEffectScope & Scope )
{
    Focused = NewFocus;

    Scope.AddFinal( this, []( UPromptBeckonInteractorComponent * Interactor )
    {
        if ( Interactor->ToldFocus.HasSameIndexAndSerialNumber( Interactor->Focused ) )
        {
            return;
        }

        const TWeakObjectPtr< UPromptBeckonInteractorComponent > Self( Interactor );
        const TWeakObjectPtr< UPromptBeckonPointComponent > Before = Interactor->ToldFocus;
        const TWeakObjectPtr< UPromptBeckonPointComponent > After  = Interactor->Focused;
        Interactor->ToldFocus                                      = After;

        if ( UPromptBeckonPointComponent * Old = Before.Get() )
        {
            Old->OnFocusChanged.Broadcast( Old, false );
        }

        if ( UPromptBeckonPointComponent * New = After.Get() )
        {
            New->OnFocusChanged.Broadcast( New, true );
        }

        if ( UPromptBeckonInteractorComponent * Told = Self.Get( true ) )
        {
            Told->OnFocusChanged.Broadcast( Before.Get(), After.Get() );
        }
    } );
}

TArray< FPromptBeckonPromptView > & UPromptBeckonInteractorComponent::FPendingPresent::GetShare( EPromptBeckonPresenterKind Kind )
{
    return Kind == EPromptBeckonPresenterKind::World ? WorldPrompts : ScreenPrompts;
}

UPromptBeckonPresenter * UPromptBeckonInteractorComponent::GetPresenter( EPromptBeckonPresenterKind Kind ) const
{
    return Kind == EPromptBeckonPresenterKind::World ? WorldPresenter.Get() : ScreenPresenter.Get();
}

TWeakObjectPtr< APlayerController > & UPromptBeckonInteractorComponent::GetViewer( EPromptBeckonPresenterKind Kind )
{
    return Kind == EPromptBeckonPresenterKind::World ? WorldViewer : ScreenViewer;
}

bool UPromptBeckonInteractorComponent::HasBegun() const
{
    return !ScreenViewer.IsExplicitlyNull() || !WorldViewer.IsExplicitlyNull();
}

void UPromptBeckonInteractorComponent::AddSync( FPromptBeckonEffectScope & Scope )
{
    Scope.AddLvl( this, []( UPromptBeckonInteractorComponent * Interactor )
    {
        TOptional< FPendingPresent > Pending = MoveTemp( Interactor->PendingPresent );
        Interactor->PendingPresent.Reset();

        const TWeakObjectPtr< UPromptBeckonInteractorComponent > Self( Interactor );
        TArray< EPromptBeckonPresenterKind > Presented;

        while ( UPromptBeckonInteractorComponent * Live = Self.Get() )
        {
            const EPromptBeckonPresenterKind * OutOfStep = Algo::FindByPredicate( PresenterKinds, [ Live ]( EPromptBeckonPresenterKind Kind )
            {
                const TWeakObjectPtr< APlayerController > & Begun = Live->GetViewer( Kind );

                return Live->GetPresenter( Kind ) && ( Begun.IsExplicitlyNull() ? Live->WantedViewer.IsValid() : !Begun.HasSameIndexAndSerialNumber( Live->WantedViewer ) );
            } );

            if ( OutOfStep )
            {
                const EPromptBeckonPresenterKind Kind       = *OutOfStep;
                UPromptBeckonPresenter * Shown              = Live->GetPresenter( Kind );
                TWeakObjectPtr< APlayerController > & Begun = Live->GetViewer( Kind );

                if ( !Begun.IsExplicitlyNull() )
                {
                    Begun.Reset();
                    Shown->End();
                }
                else
                {
                    APlayerController * Viewer = Live->WantedViewer.Get();
                    Shown->Begin( Viewer );

                    UPromptBeckonInteractorComponent * Survivor = Self.Get();
                    if ( !Survivor )
                    {
                        Shown->End();

                        return;
                    }

                    Survivor->GetViewer( Kind ) = Viewer;
                }

                if ( const UPromptBeckonInteractorComponent * Called = Self.Get(); Called && Called->bSuspended )
                {
                    Pending.Reset();
                }

                continue;
            }

            if ( !Pending )
            {
                return;
            }

            const EPromptBeckonPresenterKind * Due = Algo::FindByPredicate( PresenterKinds, [ Live, &Presented ]( EPromptBeckonPresenterKind Kind )
            {
                return !Presented.Contains( Kind ) && Live->GetPresenter( Kind ) && !Live->GetViewer( Kind ).IsExplicitlyNull();
            } );

            if ( !Due )
            {
                return;
            }

            Presented.Add( *Due );
            Live->GetPresenter( *Due )->Present( Pending->GetShare( *Due ), Pending->View, Pending->DeltaSeconds );

            if ( const UPromptBeckonInteractorComponent * Called = Self.Get(); Called && Called->bSuspended )
            {
                Pending.Reset();
            }
        }
    } );
}

void UPromptBeckonInteractorComponent::LoseViewer( FPromptBeckonEffectScope & Scope )
{
    if ( UPromptBeckonPointComponent * Held = GetHeld() )
    {
        Held->Cancel( this, PromptBeckon::Tags::ResultCancelledViewerLost );
    }

    WantedViewer.Reset();
    NumDetected = 0;
    AddSync( Scope );
    SetFocus( nullptr, Scope );
}

APlayerController * UPromptBeckonInteractorComponent::FindViewer() const
{
    const APawn * Pawn         = Cast< APawn >( GetOwner() );
    APlayerController * Player = Pawn ? Cast< APlayerController >( Pawn->GetController() ) : nullptr;

    return Player && Player->IsLocalController() ? Player : nullptr;
}

void UPromptBeckonInteractorComponent::GatherPoints( TArray< UPromptBeckonPointComponent * > & OutPoints ) const
{
    const AActor * Owner = GetOwner();
    const UWorld * World = GetWorld();
    if ( !Owner || !World )
    {
        return;
    }

    const FCollisionQueryParams Params( SCENE_QUERY_STAT( PromptBeckonGather ), false, Owner );

    TArray< FOverlapResult > Overlaps;
    World->OverlapMultiByChannel( Overlaps, Owner->GetActorLocation() + DetectOffset, FQuat::Identity, PromptBeckon::DetectChannel(), FCollisionShape::MakeSphere( DetectRadius ), Params );

    TMap< const AActor *, TArray< UPromptBeckonPointComponent * > > Listed;
    for ( const FOverlapResult & Overlap : Overlaps )
    {
        const AActor * Actor = Overlap.GetActor();
        if ( !IsValid( Actor ) || Listed.Contains( Actor ) )
        {
            continue;
        }

        TArray< UPromptBeckonPointComponent * > Asked;
        if ( Actor->Implements< UPromptBeckonPointProvider >() )
        {
            Asked = IPromptBeckonPointProvider::Execute_GetPoints( Actor );
        }

        Listed.Add( Actor, MoveTemp( Asked ) );
    }

    for ( const FOverlapResult & Overlap : Overlaps )
    {
        const AActor * Actor                                   = Overlap.GetActor();
        const UPrimitiveComponent * Shape                      = Overlap.GetComponent();
        const TArray< UPromptBeckonPointComponent * > * Points = IsValid( Actor ) && Shape ? Listed.Find( Actor ) : nullptr;
        if ( !Points )
        {
            continue;
        }

        for ( UPromptBeckonPointComponent * Point : *Points )
        {
            if ( IsValid( Point ) && Point->GetShape() == Shape )
            {
                OutPoints.AddUnique( Point );
            }
        }
    }
}

FPromptBeckonFocusCandidate UPromptBeckonInteractorComponent::MakeCand( const UPromptBeckonPointComponent * Point, const FPromptBeckonFocusView & View, const APlayerController * Viewer ) const
{
    FPromptBeckonFocusCandidate Cand;
    Cand.Location      = Point->GetComponentLocation();
    Cand.WorldDistance = FVector::Dist( View.Location, Cand.Location );
    Cand.Definition    = Point->GetDefinition();

    int32 ViewW = 0;
    int32 ViewH = 0;
    Viewer->GetViewportSize( ViewW, ViewH );

    FVector2D ScreenPos = FVector2D::ZeroVector;
    if ( ViewW > 0 && ViewH > 0 && Viewer->ProjectWorldLocationToScreen( Cand.Location, ScreenPos ) )
    {
        const FVector2D Centre( ViewW * 0.5, ViewH * 0.5 );

        Cand.bOnScreen      = true;
        Cand.ScreenDistance = static_cast< float >( FVector2D::Distance( ScreenPos, Centre ) / ViewH );
    }

    return Cand;
}

bool UPromptBeckonInteractorComponent::HasLOS( const FVector & ViewPos, const UPromptBeckonPointComponent * Point ) const
{
    const UWorld * World = GetWorld();
    if ( !World )
    {
        return false;
    }

    FCollisionQueryParams Params( SCENE_QUERY_STAT( PromptBeckonLineOfSight ), false, GetOwner() );
    Params.AddIgnoredActor( Point->GetOwner() );

    FHitResult Hit;

    return !World->LineTraceSingleByChannel( Hit, ViewPos, Point->GetComponentLocation(), PromptBeckon::OccludeChannel(), Params );
}

#if ENABLE_VISUAL_LOG
void UPromptBeckonInteractorComponent::GrabDebugSnapshot( FVisualLogEntry * Snapshot ) const
{
    FVisualLogStatusCategory & Category = Snapshot->Status.AddDefaulted_GetRef();
    Category.Category                   = TEXT( "Prompt Beckon Interactor" );

    Category.Add( TEXT( "Viewer" ), GetNameSafe( WantedViewer.Get() ) );
    Category.Add( TEXT( "Suspended" ), bSuspended ? TEXT( "Yes" ) : TEXT( "No" ) );
    Category.Add( TEXT( "Detect Radius" ), FString::Printf( TEXT( "%.1f" ), DetectRadius ) );
    Category.Add( TEXT( "Stickiness" ), FString::Printf( TEXT( "%.2f" ), Stickiness ) );
    Category.Add( TEXT( "Scorer" ), GetNameSafe( Scorer.GetScriptStruct() ) );
    Category.Add( TEXT( "Screen Presenter" ), GetNameSafe( ScreenPresenter ? ScreenPresenter->GetClass() : nullptr ) );
    Category.Add( TEXT( "World Presenter" ), GetNameSafe( WorldPresenter ? WorldPresenter->GetClass() : nullptr ) );
    Category.Add( TEXT( "Detected" ), FString::Printf( TEXT( "%d" ), NumDetected ) );
    Category.Add( TEXT( "Focused" ), GetPathNameSafe( Focused.Get() ) );

    if ( const UPromptBeckonPointComponent * Held = GetHeld() )
    {
        Category.Add( TEXT( "Session" ), FString::Printf( TEXT( "%d on %s" ), Held->GetSessionId().Value, *GetPathNameSafe( Held ) ) );
        Category.Add( TEXT( "Progress" ), FString::Printf( TEXT( "%.0f%%" ), Held->GetProgress() * 100.f ) );
    }
    else
    {
        Category.Add( TEXT( "Session" ), TEXT( "None" ) );
    }
}
#endif
