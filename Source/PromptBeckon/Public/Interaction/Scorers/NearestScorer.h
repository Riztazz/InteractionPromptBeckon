#pragma once

#include "Interaction/Scorers/FocusScorer.h"
#include "NearestScorer.generated.h"

/**
 * Usage:
 * 1. Pick Nearest as the Scorer of the interactor, for a game with no aim.
 * 2. The point nearest the camera then takes focus.
 */
USTRUCT( DisplayName = "Nearest" )
struct PROMPTBECKON_API FPromptBeckonNearestScorer : public FPromptBeckonFocusScorer
{
    GENERATED_BODY()

    FPromptBeckonNearestScorer();

    virtual bool ScoreCand( const FPromptBeckonFocusView & View, const FPromptBeckonFocusCandidate & Cand, float & OutCost ) const override;
};
