//
//  PartialsToFreq2.h
//  BL-SASViewer
//
//  Created by applematuer on 2/25/19.
//
//

#ifndef __BL_SASViewer__PartialsToFreq2__
#define __BL_SASViewer__PartialsToFreq2__

#include <vector>
using namespace std;

#include <PartialTracker3.h>

// PartialsToFreq: original code, moved from SASFrame to here
// PartialsToFreq2: improved algorithm
//
class PartialTWMEstimate2;
class PartialsToFreq2
{
public:
    PartialsToFreq2(int bufferSize, BL_FLOAT sampleRate);
    
    virtual ~PartialsToFreq2();
    
    void Reset(BL_FLOAT sampleRate);
    
    BL_FLOAT ComputeFrequency(const vector<PartialTracker3::Partial> &partials);
    
protected:
    void GetBestPartials(vector<PartialTracker3::Partial> *partials);
    
    void ComputeAgeScores(const vector<PartialTracker3::Partial> &partials,
                          vector<BL_FLOAT> *ageScores);

    void ComputeAmpScores(const vector<PartialTracker3::Partial> &partials,
                          vector<BL_FLOAT> *ampScores);
    
    void ComputeFreqScores(const vector<PartialTracker3::Partial> &partials,
                           vector<BL_FLOAT> *freqScores);
    
    //
    void ThresholdPartials(vector<PartialTracker3::Partial> *partials);

    //
    void ThresholdPartialsRelative(vector<PartialTracker3::Partial> *partials);
    
    //
    void SelectAlivePartials(vector<PartialTracker3::Partial> *partials);

    
    PartialTWMEstimate2 *mFreqEstimate;
};

#endif /* defined(__BL_SASViewer__PartialsToFreq2__) */
