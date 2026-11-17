#include "Interaction/Scorers/NearestScorer.h"

FPromptBeckonNearestScorer::FPromptBeckonNearestScorer()
{
    MaxCost = 1.f;
}

bool FPromptBeckonNearestScorer::ScoreCand( const FPromptBeckonFocusView & View, const FPromptBeckonFocusCandidate & Cand, float & OutCost ) const
{
    if ( View.DetectRadius <= 0.f )
    {
        return false;
    }

    OutCost = Cand.WorldDistance / View.DetectRadius;
    return true;
}
