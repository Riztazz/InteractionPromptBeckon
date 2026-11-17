#include "Interaction/Scorers/FocusScorer.h"

#include "Math/UnrealMathUtility.h"

namespace
{
    constexpr float MaxStick = 0.95f;
}

bool FPromptBeckonFocusScorer::ScoreCand( const FPromptBeckonFocusView & View, const FPromptBeckonFocusCandidate & Cand, float & OutCost ) const
{
    return false;
}

int32 FPromptBeckonFocusScorer::PickFocus( TConstArrayView< TOptional< float > > Costs, int32 CurIdx, float Stickiness, float MaxCost )
{
    const float KeepFactor = 1.f - FMath::Clamp( Stickiness, 0.f, MaxStick );

    int32 BestIndex = INDEX_NONE;
    float BestCost  = 0.f;

    for ( int32 Idx = 0; Idx < Costs.Num(); ++Idx )
    {
        if ( !Costs[ Idx ].IsSet() || !FMath::IsFinite( Costs[ Idx ].GetValue() ) )
        {
            continue;
        }

        const float Effective = FMath::Max( Costs[ Idx ].GetValue(), 0.f ) * ( Idx == CurIdx ? KeepFactor : 1.f );
        if ( Effective > MaxCost )
        {
            continue;
        }

        const bool bBeatsBest = BestIndex == INDEX_NONE || Effective < BestCost || ( Effective == BestCost && Idx == CurIdx );
        if ( bBeatsBest )
        {
            BestIndex = Idx;
            BestCost  = Effective;
        }
    }

    return BestIndex;
}
