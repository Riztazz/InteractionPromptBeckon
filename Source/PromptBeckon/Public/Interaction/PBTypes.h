#pragma once

#include "Interaction/Drivers/InteractionDriver.h"
#include "Internationalization/Text.h"
#include "Logging/LogMacros.h"
#include "Math/Rotator.h"
#include "Math/Transform.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Vector.h"
#include "UObject/ObjectPtr.h"
#include "UObject/WeakObjectPtr.h"
#include "PBTypes.generated.h"

class UPromptBeckonDefinition;
class UPromptBeckonInteractorComponent;
class UPromptBeckonPointComponent;

DECLARE_LOG_CATEGORY_EXTERN( LogPromptBeckon, Log, All );

UENUM( BlueprintType )
enum class EPromptBeckonPointState : uint8
{
    Available,
    InProgress,
    Cooldown
};

UENUM( BlueprintType )
enum class EPromptBeckonAvailability : uint8
{
    Available,
    Refused,
    Hidden
};

UENUM( BlueprintType )
enum class EPromptBeckonPromptVisibility : uint8
{
    Hidden,
    Disabled,
    Enabled
};

UENUM( BlueprintType )
enum class EPromptBeckonPresenterKind : uint8
{
    Screen,
    World
};

USTRUCT( BlueprintType )
struct PROMPTBECKON_API FPromptBeckonFocusLook
{
    GENERATED_BODY()

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Focus", meta = ( ClampMin = "0.01" ) )
    float UnfocusScale = 0.5f;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Focus", meta = ( ClampMin = "0.01" ) )
    float FocusScale = 1.25f;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Focus", meta = ( ClampMin = "0.0", ClampMax = "1.0" ) )
    float UnfocusOpacity = 0.25f;

    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Focus", meta = ( ClampMin = "0.1" ) )
    float InterpSpeed = 12.f;

    float ScaleAt( float Alpha ) const
    {
        return FMath::Lerp( UnfocusScale, FocusScale, Alpha );
    }

    float OpacityAt( float Alpha ) const
    {
        return FMath::Lerp( UnfocusOpacity, 1.f, Alpha );
    }
};

USTRUCT( BlueprintType )
struct PROMPTBECKON_API FPromptBeckonPromptState
{
    GENERATED_BODY()

    UPROPERTY( BlueprintReadOnly, Category = "Prompt" )
    FText Verb;

    UPROPERTY( BlueprintReadOnly, Category = "Prompt" )
    float Progress = 0.f;

    UPROPERTY( BlueprintReadOnly, Category = "Prompt" )
    float FocusAlpha = 0.f;

    UPROPERTY( BlueprintReadOnly, Category = "Prompt" )
    bool bEnabled = true;

    bool operator==( const FPromptBeckonPromptState & Other ) const
    {
        return Verb.IdenticalTo( Other.Verb ) && Progress == Other.Progress && FocusAlpha == Other.FocusAlpha && bEnabled == Other.bEnabled;
    }

    bool operator!=( const FPromptBeckonPromptState & Other ) const
    {
        return !( *this == Other );
    }
};

USTRUCT( BlueprintType )
struct PROMPTBECKON_API FPromptBeckonFocusView
{
    GENERATED_BODY()

    UPROPERTY( BlueprintReadOnly, Category = "Focus" )
    FVector Location = FVector::ZeroVector;

    UPROPERTY( BlueprintReadOnly, Category = "Focus" )
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY( BlueprintReadOnly, Category = "Focus" )
    float DetectRadius = 0.f;
};

USTRUCT( BlueprintType )
struct PROMPTBECKON_API FPromptBeckonFocusCandidate
{
    GENERATED_BODY()

    UPROPERTY( BlueprintReadOnly, Category = "Focus" )
    FVector Location = FVector::ZeroVector;

    UPROPERTY( BlueprintReadOnly, Category = "Focus" )
    bool bOnScreen = false;

    UPROPERTY( BlueprintReadOnly, Category = "Focus" )
    float ScreenDistance = 0.f;

    UPROPERTY( BlueprintReadOnly, Category = "Focus" )
    float WorldDistance = 0.f;

    UPROPERTY( BlueprintReadOnly, Category = "Focus" )
    TObjectPtr< const UPromptBeckonDefinition > Definition;
};

struct FPromptBeckonPromptView
{
    TWeakObjectPtr< const UPromptBeckonPointComponent > Point;
    FTransform Anchor                          = FTransform::Identity;
    const UPromptBeckonDefinition * Definition = nullptr;
    float Progress                             = 0.f;
    bool bFocused                              = false;
    bool bEnabled                              = true;
};

USTRUCT( BlueprintType )
struct FPromptBeckonInteractionSessionId
{
    GENERATED_BODY()

    UPROPERTY( BlueprintReadOnly, Category = "Interaction" )
    int32 Value = 0;

    bool IsSet() const
    {
        return Value != 0;
    }

    bool operator==( const FPromptBeckonInteractionSessionId & Other ) const
    {
        return Value == Other.Value;
    }

    bool operator!=( const FPromptBeckonInteractionSessionId & Other ) const
    {
        return !( *this == Other );
    }

    static PROMPTBECKON_API FPromptBeckonInteractionSessionId Next();
};

USTRUCT()
struct FPromptBeckonInteractionSession
{
    GENERATED_BODY()

    FPromptBeckonInteractionSessionId Id;
    TWeakObjectPtr< UPromptBeckonInteractorComponent > Holder;

    UPROPERTY( Transient )
    TObjectPtr< const UPromptBeckonDefinition > Definition;

    FPromptBeckonDriverRun Run;

    bool IsSet() const
    {
        return Id.IsSet();
    }

    void Reset()
    {
        Id = FPromptBeckonInteractionSessionId();
        Holder.Reset();
        Definition = nullptr;
        Run        = FPromptBeckonDriverRun();
    }
};
