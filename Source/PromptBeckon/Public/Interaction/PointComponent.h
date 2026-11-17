#pragma once

#include "Components/SceneComponent.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"
#include "Interaction/Drivers/InteractionDriver.h"
#include "Interaction/PBTypes.h"
#include "Internationalization/Text.h"
#include "UObject/ObjectPtr.h"
#include "PointComponent.generated.h"

class FPromptBeckonEffectScope;
class UPrimitiveComponent;
class UPromptBeckonDefinition;
class UPromptBeckonInteractorComponent;
class UPromptBeckonPointComponent;

UENUM()
enum class EPromptBeckonShapeProblem : uint8
{
    None,
    NoShape,
    QueryCollisionOff,
    IgnoresDetectionChannel,
    NoProvider
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams( FPromptBeckonPointInteracted, UPromptBeckonPointComponent *, Point, UPromptBeckonInteractorComponent *, Interactor );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams( FPromptBeckonPointInteractionEnded, UPromptBeckonPointComponent *, Point, FGameplayTag, Result );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams( FPromptBeckonPointStateChanged, UPromptBeckonPointComponent *, Point, EPromptBeckonPointState, OldState, EPromptBeckonPointState, NewState );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams( FPromptBeckonPointAvailabilityChanged, UPromptBeckonPointComponent *, Point, EPromptBeckonAvailability, OldAvail, EPromptBeckonAvailability, NewAvail );
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams( FPromptBeckonPointFocusChanged, UPromptBeckonPointComponent *, Point, bool, bFocused );

/**
 * Usage:
 * 1. Add a sphere or box component to the actor and pass it to ConfigShape.
 * 2. Attach this point to that shape.
 * 3. Set its Definition.
 * 4. Bind OnInteracted to do the action.
 * 5. Call SetDefinition to change the prompt, such as Open to Close.
 * 6. Call SetAvail to block or hide it.
 */
UCLASS( ClassGroup = ( Interaction ), meta = ( BlueprintSpawnableComponent ) )
class PROMPTBECKON_API UPromptBeckonPointComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UPromptBeckonPointComponent();

    UPROPERTY( BlueprintAssignable, Category = "Prompt Beckon" )
    FPromptBeckonPointInteracted OnInteracted;

    UPROPERTY( BlueprintAssignable, Category = "Prompt Beckon" )
    FPromptBeckonPointInteractionEnded OnInteractEnd;

    UPROPERTY( BlueprintAssignable, Category = "Prompt Beckon" )
    FPromptBeckonPointStateChanged OnStateChanged;

    UPROPERTY( BlueprintAssignable, Category = "Prompt Beckon" )
    FPromptBeckonPointAvailabilityChanged OnAvailChanged;

    UPROPERTY( BlueprintAssignable, Category = "Prompt Beckon" )
    FPromptBeckonPointFocusChanged OnFocusChanged;

    UFUNCTION( BlueprintPure, Category = "Prompt Beckon" )
    UPrimitiveComponent * GetShape() const;

    UFUNCTION( BlueprintCallable, Category = "Prompt Beckon" )
    static void ConfigShape( UPrimitiveComponent * Shape );

    static EPromptBeckonShapeProblem FindShapeErr( const USceneComponent * AttachParent );
    static FText GetShapeErrText( EPromptBeckonShapeProblem Problem );
    static bool IsShapeErr( EPromptBeckonShapeProblem Problem );

#if WITH_EDITOR
    virtual void CheckForErrors() override;
#endif

    UFUNCTION( BlueprintCallable, Category = "Prompt Beckon" )
    void SetDefinition( const UPromptBeckonDefinition * NewDefinition );

    UFUNCTION( BlueprintPure, Category = "Prompt Beckon" )
    const UPromptBeckonDefinition * GetDefinition() const;

    UFUNCTION( BlueprintCallable, Category = "Prompt Beckon" )
    void SetAvail( EPromptBeckonAvailability NewAvail, const FText & Reason );

    UFUNCTION( BlueprintPure, Category = "Prompt Beckon" )
    EPromptBeckonAvailability GetAvail() const;

    UFUNCTION( BlueprintPure, Category = "Prompt Beckon" )
    const FText & GetUnavailReason() const;

    UFUNCTION( BlueprintCallable, Category = "Prompt Beckon" )
    void ResetAvail();

    UFUNCTION( BlueprintPure, Category = "Prompt Beckon" )
    EPromptBeckonPointState GetState() const;

    UFUNCTION( BlueprintPure, Category = "Prompt Beckon" )
    bool CanInteract() const;

    EPromptBeckonPromptVisibility GetVis() const;

    static EPromptBeckonPromptVisibility CalcVis( EPromptBeckonPointState State, EPromptBeckonAvailability Availability, bool bDefValid, bool bHideOnCooldown );

    bool Press( UPromptBeckonInteractorComponent * Interactor );
    void Release( const UPromptBeckonInteractorComponent * Interactor );
    void Advance( const UPromptBeckonInteractorComponent * Interactor, float DeltaSeconds );
    void Cancel( const UPromptBeckonInteractorComponent * Interactor, FGameplayTag Result );
    bool IsHeldBy( const UPromptBeckonInteractorComponent * Interactor ) const;
    float GetProgress() const;
    bool AllowsUse( const UPromptBeckonInteractorComponent * Interactor ) const;
    FPromptBeckonInteractionSessionId GetSessionId() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay( EEndPlayReason::Type EndPlayReason ) override;

private:
    UPROPERTY( EditAnywhere, Category = "Prompt Beckon" )
    TObjectPtr< const UPromptBeckonDefinition > Definition;

    EPromptBeckonPointState State          = EPromptBeckonPointState::Available;
    EPromptBeckonAvailability Availability = EPromptBeckonAvailability::Available;
    FText UnavailableReason;

    UPROPERTY( Transient )
    FPromptBeckonInteractionSession Session;

    FTimerHandle CooldownTimer;

    void SetState( EPromptBeckonPointState NewState, FPromptBeckonEffectScope & Scope );
    void AddState( EPromptBeckonPointState OldState, EPromptBeckonPointState NewState, FPromptBeckonEffectScope & Scope );
    void ApplyStep( const FPromptBeckonDriverStep & Step, FPromptBeckonEffectScope & Scope );
    void EndSession( FGameplayTag Result, bool bCompleted, EPromptBeckonPointState NewState, FPromptBeckonEffectScope & Scope );
    bool ApplyPress( const UPromptBeckonInteractorComponent * Interactor, FPromptBeckonEffectScope & Scope );
    const FPromptBeckonDriver * ResolveDriver( FPromptBeckonEffectScope & Scope );
    float GetCooldown() const;
    bool AcceptsHolder( const UPromptBeckonInteractorComponent * Interactor, const TCHAR * Call ) const;
    void OnCooldownDone();
};
