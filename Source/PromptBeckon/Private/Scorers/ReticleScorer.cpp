#include "Interaction/Scorers/ReticleScorer.h"

bool FPromptBeckonReticleScorer::ScoreCand( const FPromptBeckonFocusView & View, const FPromptBeckonFocusCandidate & Cand, float & OutCost ) const
{
    if ( !Cand.bOnScreen )
    {
        return false;
    }

    OutCost = Cand.ScreenDistance;
    return true;
}
