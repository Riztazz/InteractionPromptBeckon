#pragma once

#include "Interaction/Scorers/FocusScorer.h"
#include "ReticleScorer.generated.h"

/**
 * Usage:
 * 1. Pick Reticle as the Scorer of the interactor. It is the default.
 * 2. The point nearest the screen centre then takes focus.
 * 3. Raise MaxCost to let points farther from the centre take focus.
 */
USTRUCT( DisplayName = "Reticle" )
struct PROMPTBECKON_API FPromptBeckonReticleScorer : public FPromptBeckonFocusScorer
{
    GENERATED_BODY()

    virtual bool ScoreCand( const FPromptBeckonFocusView & View, const FPromptBeckonFocusCandidate & Cand, float & OutCost ) const override;
};
