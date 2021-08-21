#ifndef PARTIAL_FILTER_AMFM_H
#define PARTIAL_FILTER_AMFM_H

#include <vector>
#include <deque>
using namespace std;

#include <PartialFilter.h>

// Method using alhpa0 and dlta0 from AM FM paper
// https://www.researchgate.net/publication/235219224_Improved_partial_tracking_technique_for_sinusoidal_modeling_of_speech_and_audio
// Also use zonbied as suggested
class PartialFilterAMFM : public PartialFilter
{
 public:
    PartialFilterAMFM(int bufferSize, BL_FLOAT smapleRate);
    virtual ~PartialFilterAMFM();

    void Reset(int bufferSize, BL_FLOAT smapleRate);
        
    void FilterPartials(vector<Partial> *partials);

 protected:
    // Method based on alpha0 and beta0
    void AssociatePartialsAMFM(const vector<Partial> &prevPartials,
                               vector<Partial> *currentPartials,
                               vector<Partial> *remainingCurrentPartials);

    void ComputeZombieDeadPartials(const vector<Partial> &prevPartials,
                                   const vector<Partial> &currentPartials,
                                   vector<Partial> *zombieDeadPartials);

    void FixPartialsCrossing(const vector<Partial> &partials0,
                             const vector<Partial> &partials1,
                             vector<Partial> *partials2);
        
    int FindPartialById(const vector<Partial> &partials, int idx);

    BL_FLOAT ComputeLA(const Partial &currentPartial, const Partial &otherPartial);
    BL_FLOAT ComputeLF(const Partial &currentPartial, const Partial &otherPartial);
    BL_FLOAT ComputeTrapezoidArea(BL_FLOAT a, BL_FLOAT b, BL_FLOAT c, BL_FLOAT d);

    void ExtrapolatePartialAMFM(Partial *p);
    void ExtrapolatePartialKalman(Partial *p);
    
    void DBG_PrintPartials(const vector<Partial> &partials);
    
    //
    deque<vector<Partial> > mPartials;

    int mBufferSize;
    BL_FLOAT mSampleRate;
    
 private:
    vector<Partial> mTmpPartials0;
    vector<Partial> mTmpPartials1;
    vector<Partial> mTmpPartials2;
    vector<Partial> mTmpPartials3;
    vector<Partial> mTmpPartials4;
    vector<Partial> mTmpPartials5;
    vector<Partial> mTmpPartials6;
    vector<Partial> mTmpPartials7;
};

#endif
