//
//  PartialsToFreq5.h
//  BL-SASViewer
//
//  Created by applematuer on 2/25/19.
//
//

#ifndef __BL_SASViewer__PartialsToFreq5__
#define __BL_SASViewer__PartialsToFreq5__

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
class PartialTWMEstimate3;
class PartialsToFreq5
{
public:
    PartialsToFreq5(int bufferSize, BL_FLOAT sampleRate);
    
    virtual ~PartialsToFreq5();
    
    void SetHarmonicSoundFlag(bool flag);
    
    BL_FLOAT ComputeFrequency(const vector<PartialTracker5::Partial> &partials);
    
protected:
    BL_FLOAT AdjustFreqToPartial(BL_FLOAT freq,
                                 const vector<PartialTracker5::Partial> &partials);

    BL_FLOAT AdjustFreqToPartialOctave(BL_FLOAT freq,
                                     const vector<PartialTracker5::Partial> &partials);
    
    void ThresholdPartials(vector<PartialTracker5::Partial> *partials);
    void ThresholdPartialsRelative(vector<PartialTracker5::Partial> *partials);

    //
    int mBufferSize;
    BL_FLOAT mSampleRate;
    
    PartialTWMEstimate3 *mEstimate;
};

#endif /* defined(__BL_SASViewer__PartialsToFreq5__) */
