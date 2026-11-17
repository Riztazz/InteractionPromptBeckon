#pragma once

#include "Containers/ArrayView.h"
#include "Interaction/PBTypes.h"
#include "Misc/Optional.h"
#include "UObject/ObjectMacros.h"
#include "FocusScorer.generated.h"

/**
 * Usage:
 * 1. Derive a struct from it to pick the focus your own way.
 * 2. Override ScoreCand. The lowest cost wins.
 * 3. Pick it as the Scorer of the interactor.
 */
USTRUCT()
struct PROMPTBECKON_API FPromptBeckonFocusScorer
{
    GENERATED_BODY()

    virtual ~FPromptBeckonFocusScorer() = default;

    UPROPERTY( EditAnywhere, Category = "Focus", meta = ( ClampMin = "0.0" ) )
    float MaxCost = 0.15f;

    virtual bool ScoreCand( const FPromptBeckonFocusView & View, const FPromptBeckonFocusCandidate & Cand, float & OutCost ) const;

    static int32 PickFocus( TConstArrayView< TOptional< float > > Costs, int32 CurIdx, float Stickiness, float MaxCost );
};
