#ifndef PARTIAL_FILTER_AMFM_H
#define PARTIAL_FILTER_AMFM_H

#include <vector>
#include <deque>
using namespace std;

#include <PartialFilter.h>

// Method using alhpa0 and dlta0 from AM FM paper
// https://www.researchgate.net/publication/235219224_Improved_partial_tracking_technique_for_sinusoidal_modeling_of_speech_and_audio
// Also use zombied as suggested
class PartialFilterAMFM : public PartialFilter
{
 public:
    PartialFilterAMFM(int bufferSize, BL_FLOAT sampleRate);
    virtual ~PartialFilterAMFM();

    void Reset(int bufferSize, BL_FLOAT smapleRate);
        
    void FilterPartials(vector<Partial> *partials);

    void SetNeriDelta(BL_FLOAT delta);
    
 protected:
    // Method based on alpha0 and beta0 (not optimized)
    void AssociatePartialsAMFMSimple(const vector<Partial> &prevPartials,
                                     vector<Partial> *currentPartials,
                                     vector<Partial> *remainingCurrentPartials);

    // Method based on alpha0 and beta0 (optimized)
    void AssociatePartialsAMFM(const vector<Partial> &prevPartials,
                               vector<Partial> *currentPartials,
                               vector<Partial> *remainingCurrentPartials);
    long FindNearestFreqId(const vector<Partial> &partials,
                           BL_FLOAT freq, int index);
        
    // Compute score using Neri
    void AssociatePartialsNeri(const vector<Partial> &prevPartials,
                               vector<Partial> *currentPartials,
                               vector<Partial> *remainingCurrentPartials);
    
    void AssociatePartialsHungarianAMFM(const vector<Partial> &prevPartials,
                                        vector<Partial> *currentPartials,
                                        vector<Partial> *remainingCurrentPartials);

    // Compute score like in the paper
    void AssociatePartialsHungarianNeri(const vector<Partial> &prevPartials,
                                        vector<Partial> *currentPartials,
                                        vector<Partial> *remainingCurrentPartials);
    
    void ComputeZombieDeadPartials(const vector<Partial> &prevPartials,
                                   const vector<Partial> &currentPartials,
                                   vector<Partial> *zombieDeadPartials);

    void FixPartialsCrossing(const vector<Partial> &partials0,
                             const vector<Partial> &partials1,
                             vector<Partial> *partials2);
        
    int FindPartialById(const vector<Partial> &partials, int idx);
    // Optimized
    int FindPartialByIdSorted(const vector<Partial> &partials,
                              const Partial &refPartial);
    
    // For AMFM
    BL_FLOAT ComputeLA(const Partial &prevPartial, const Partial &currentPartial);
    BL_FLOAT ComputeLF(const Partial &prevPartial, const Partial &currentPartial);

    // for hungarian Neri
    void ComputeCostNeri(const Partial &prevPartial,
                         const Partial &currentPartial,
                         BL_FLOAT delta, BL_FLOAT zetaF, BL_FLOAT zetaA,
                         BL_FLOAT *A, BL_FLOAT *B);
        
    void ExtrapolatePartialAMFM(Partial *p);
    void ExtrapolatePartialKalman(Partial *p);

    bool CheckDiscardBigJump(const Partial &prevPartial,
                             const Partial &currentPartial);
    bool CheckDiscardOppositeDirection(const Partial &prevPartial,
                                       const Partial &currentPartial);
    
    //
    deque<vector<Partial> > mPartials;

    int mBufferSize;
    BL_FLOAT mSampleRate;

    BL_FLOAT mNeriDelta;
    
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
