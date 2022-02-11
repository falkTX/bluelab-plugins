//
//  PartialsDiffEstimate.h
//  BL-SASViewer
//
//  Created by applematuer on 2/25/19.
//
//

#ifndef __BL_SASViewer__PartialsDiffEstimate__
#define __BL_SASViewer__PartialsDiffEstimate__

#include <vector>
using namespace std;

#include <PartialTracker3.h>

// PartialsToFreq: original code, moved from SASFrame to here
// PartialsToFreq2: improved algorithm
//
// PartialsToFreq3: custom algo, use freq diffs and sigma
//
// PartialsDifffEstimate: keep only the diff algorithm (custom algorithm)

class PartialsDiffEstimate
{
public:
    PartialsDiffEstimate(int bufferSize, BL_FLOAT sampleRate);
    
    virtual ~PartialsDiffEstimate();
    
    BL_FLOAT ComputeFrequency(const vector<PartialTracker3::Partial> &partials);
    
protected:
    // First method
    BL_FLOAT ComputeFreqDiff(const vector<PartialTracker3::Partial> &partials);
    
    BL_FLOAT ComputeMinFreqDiff(const vector<PartialTracker3::Partial> &partials);
    
    // Auxiliary
    void GenerateHarmonics(vector<PartialTracker3::Partial> *partials);

    void GenerateOctaves(vector<PartialTracker3::Partial> *partials);
    
    //
    BL_FLOAT AdjustFreqToPartial(BL_FLOAT freq,
                               const vector<PartialTracker3::Partial> &partials);

    BL_FLOAT AdjustFreqToPartialOctave(BL_FLOAT freq,
                                     const vector<PartialTracker3::Partial> &partials);
    
    //
    void ThresholdPartials(vector<PartialTracker3::Partial> *partials);

    //
    void ThresholdPartialsRelative(vector<PartialTracker3::Partial> *partials);
    
    //
    void SelectAlivePartials(vector<PartialTracker3::Partial> *partials);
    
    //
    int mBufferSize;
    BL_FLOAT mSampleRate;
};

#endif /* defined(__BL_SASViewer__PartialsDiffEstimate__) */
