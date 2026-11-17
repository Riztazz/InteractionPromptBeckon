#pragma once

#include "Components/ActorComponent.h"
#include "Containers/Array.h"
#include "Delegates/DelegateCombinations.h"
#include "EngineDefines.h"
#include "GameplayTagContainer.h"
#include "Interaction/PBTypes.h"
#include "Interaction/Scorers/FocusScorer.h"
#include "Math/Vector.h"
#include "Misc/Optional.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/ObjectPtr.h"
#include "UObject/WeakObjectPtr.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"
#include "InteractorComponent.generated.h"

class APlayerController;
class FPromptBeckonEffectScope;
class UPromptBeckonPointComponent;
class UPromptBeckonPresenter;
struct FVisualLogEntry;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams( FPromptBeckonFocusChanged, UPromptBeckonPointComponent *, OldFocus, UPromptBeckonPointComponent *, NewFocus );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FPromptBeckonInteractionStarted, UPromptBeckonPointComponent *, Point );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams( FPromptBeckonInteractionEnded, UPromptBeckonPointComponent *, Point, FGameplayTag, Result );

/**
 * Usage:
 * 1. Add it to the player's pawn.
 * 2. Call InteractPressed and InteractReleased from your input.
 * 3. Pick the Scorer, the ScreenPresenter and the WorldPresenter in its details.
 * 4. Bind OnFocusChanged or OnInteractEnd to react in your own code.
 */
UCLASS( ClassGroup = ( Interaction ), meta = ( BlueprintSpawnableComponent ) )
class PROMPTBECKON_API UPromptBeckonInteractorComponent : public UActorComponent, public IVisualLoggerDebugSnapshotInterface
{
    GENERATED_BODY()

public:
    UPromptBeckonInteractorComponent();

    UPROPERTY( BlueprintAssignable, Category = "Prompt Beckon" )
    FPromptBeckonFocusChanged OnFocusChanged;

    UPROPERTY( BlueprintAssignable, Category = "Prompt Beckon" )
    FPromptBeckonInteractionStarted OnInteractStart;

    UPROPERTY( BlueprintAssignable, Category = "Prompt Beckon" )
    FPromptBeckonInteractionEnded OnInteractEnd;

    UFUNCTION( BlueprintCallable, Category = "Prompt Beckon" )
    void InteractPressed();

    UFUNCTION( BlueprintCallable, Category = "Prompt Beckon" )
    void InteractReleased();

    UFUNCTION( BlueprintCallable, Category = "Prompt Beckon" )
    void SetSuspended( bool bInSuspended );

    UFUNCTION( BlueprintPure, Category = "Prompt Beckon" )
    bool IsSuspended() const;

    UFUNCTION( BlueprintPure, Category = "Prompt Beckon" )
    bool IsInteracting() const;

    UFUNCTION( BlueprintPure, Category = "Prompt Beckon" )
    UPromptBeckonPointComponent * GetFocusedPoint() const;

    bool CanClaim( const UPromptBeckonPointComponent * Point ) const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay( EEndPlayReason::Type EndPlayReason ) override;
    virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction ) override;

#if ENABLE_VISUAL_LOG
    virtual void GrabDebugSnapshot( FVisualLogEntry * Snapshot ) const override;
#endif

private:
    UPROPERTY( EditAnywhere, Category = "Prompt Beckon", meta = ( ClampMin = "10.0", ForceUnits = "cm" ) )
    float DetectRadius = 250.f;

    UPROPERTY( EditAnywhere, Category = "Prompt Beckon" )
    FVector DetectOffset = FVector::ZeroVector;

    UPROPERTY( EditAnywhere, Category = "Prompt Beckon", meta = ( ClampMin = "0.0", ClampMax = "0.95" ) )
    float Stickiness = 0.25f;

    UPROPERTY( EditAnywhere, Category = "Prompt Beckon" )
    bool bRequireLOS = false;

    UPROPERTY( EditAnywhere, Category = "Prompt Beckon", meta = ( ExcludeBaseStruct ) )
    TInstancedStruct< FPromptBeckonFocusScorer > Scorer;

    UPROPERTY( EditAnywhere, Instanced, NoClear, Category = "Prompt Beckon" )
    TObjectPtr< UPromptBeckonPresenter > ScreenPresenter;

    UPROPERTY( EditAnywhere, Instanced, NoClear, Category = "Prompt Beckon" )
    TObjectPtr< UPromptBeckonPresenter > WorldPresenter;

    struct FPendingPresent
    {
        TArray< FPromptBeckonPromptView > ScreenPrompts;
        TArray< FPromptBeckonPromptView > WorldPrompts;
        FPromptBeckonFocusView View;
        float DeltaSeconds = 0.f;

        TArray< FPromptBeckonPromptView > & GetShare( EPromptBeckonPresenterKind Kind );
    };

    TWeakObjectPtr< UPromptBeckonPointComponent > Focused;
    TWeakObjectPtr< UPromptBeckonPointComponent > ToldFocus;
    TWeakObjectPtr< APlayerController > WantedViewer;
    TWeakObjectPtr< APlayerController > ScreenViewer;
    TWeakObjectPtr< APlayerController > WorldViewer;
    TOptional< FPendingPresent > PendingPresent;
    int32 NumDetected = 0;
    bool bSuspended   = false;

    UPromptBeckonPointComponent * GetHeld() const;
    void SetFocus( UPromptBeckonPointComponent * NewFocus, FPromptBeckonEffectScope & Scope );
    UPromptBeckonPresenter * GetPresenter( EPromptBeckonPresenterKind Kind ) const;
    TWeakObjectPtr< APlayerController > & GetViewer( EPromptBeckonPresenterKind Kind );
    bool HasBegun() const;
    void AddSync( FPromptBeckonEffectScope & Scope );
    void LoseViewer( FPromptBeckonEffectScope & Scope );
    APlayerController * FindViewer() const;
    void GatherPoints( TArray< UPromptBeckonPointComponent * > & OutPoints ) const;
    FPromptBeckonFocusCandidate MakeCand( const UPromptBeckonPointComponent * Point, const FPromptBeckonFocusView & View, const APlayerController * Viewer ) const;
    bool HasLOS( const FVector & ViewPos, const UPromptBeckonPointComponent * Point ) const;
};
