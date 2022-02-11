//
//  PartialsToFreq.h
//  BL-SASViewer
//
//  Created by applematuer on 2/25/19.
//
//

#ifndef __BL_SASViewer__PartialsToFreq__
#define __BL_SASViewer__PartialsToFreq__

#include <vector>
using namespace std;

#include <PartialTracker3.h>

class PartialTWMEstimate2;

// PartialsToFreq: original code, moved from SASFrame to here
class PartialsToFreq
{
public:
    PartialsToFreq(int bufferSize, BL_FLOAT sampleRate);
    
    virtual ~PartialsToFreq();
    
    void Reset(BL_FLOAT sampleRate);
    
    BL_FLOAT ComputeFrequency(const vector<PartialTracker3::Partial> &partials);
    
protected:
    void SelectLowFreqPartials(vector<PartialTracker3::Partial> *partials);
    
    void SelectHighAmpPartials(vector<PartialTracker3::Partial> *partials);
    
    
    PartialTWMEstimate2 *mFreqEstimate;
    
    BL_FLOAT mPrevFrequency;
    int mPrevNumPartials;
};

#endif /* defined(__BL_SASViewer__PartialsToFreq__) */
