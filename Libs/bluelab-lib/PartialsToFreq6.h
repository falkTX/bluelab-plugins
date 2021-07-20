//
//  PartialsToFreq6.h
//  BL-SASViewer
//
//  Created by applematuer on 2/25/19.
//
//

#ifndef __BL_SASViewer__PartialsToFreq6__
#define __BL_SASViewer__PartialsToFreq6__

#include <vector>
using namespace std;

#include <PartialTracker5.h>

// PartialsToFreq: original code, moved from SASFrame to here
// PartialsToFreq2: improved algorithm
//
// PartialsToFreq3: custom algo, use freq diffs and sigma
//
// PartialsToFreq4: keep only the structure and moved the freq diff algo
// to a new class
// (will be useful if need filtering frequency)
//
// PartialsToFreq6: use chromagram technique, to first find the note,
// then find the partial closest to this note (to try to be more robust)
class PartialTWMEstimate3;
class ChromagramObj;
class PartialsToFreq6
{
public:
    PartialsToFreq6(int bufferSize, int oversampling,
                    int freqRes, BL_FLOAT sampleRate);
    
    virtual ~PartialsToFreq6();

    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate);
    
    void SetHarmonicSoundFlag(bool flag);
    
    BL_FLOAT ComputeFrequency(const WDL_TypedBuf<BL_FLOAT> &magns,
                              const WDL_TypedBuf<BL_FLOAT> &phases,
                              const vector<PartialTracker5::Partial> &partials);
    
protected:
    BL_FLOAT AdjustFreqToPartial(BL_FLOAT freq,
                                 const vector<PartialTracker5::Partial> &partials);

    BL_FLOAT
    AdjustFreqToPartialOctave(BL_FLOAT freq,
                              const vector<PartialTracker5::Partial> &partials);
    
    void ThresholdPartials(vector<PartialTracker5::Partial> *partials);
    void ThresholdPartialsRelative(vector<PartialTracker5::Partial> *partials);

    BL_FLOAT FindClosestPartialFreq(BL_FLOAT refFreq0,
                                    const vector<PartialTracker5::Partial> &partials);
    
    //
    int mBufferSize;
    BL_FLOAT mSampleRate;

    ChromagramObj *mChromaObj;
    
    PartialTWMEstimate3 *mEstimate; // Unused ?

private:
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
};

#endif /* defined(__BL_SASViewer__PartialsToFreq6__) */
