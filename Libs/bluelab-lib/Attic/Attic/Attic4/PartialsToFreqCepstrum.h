//
//  PartialsToFreqCepstrum.h
//  BL-SASViewer
//
//  Created by applematuer on 2/25/19.
//
//

#ifndef __BL_SASViewer__PartialsToFreqCepstrum__
#define __BL_SASViewer__PartialsToFreqCepstrum__

#include <vector>
using namespace std;

#include <PartialTracker3.h>

// PartialsToFreq: original code, moved from SASFrame to here
// PartialsToFreq2: improved algorithm
//
// PartialsToFreqCepstrum: new algorithm
//
// See: https://www.johndcook.com/blog/2016/05/18/cepstrum-quefrency-and-pitch/
// and: http://kom.aau.dk/group/04gr742/pdf/cepstrum.pdf
// and: https://stackoverflow.com/questions/7813213/estimate-the-fundamental-frequency-using-cepstral-analysis
// and: https://en.wikipedia.org/wiki/Cepstrum

// NOTE: usually cepstrum is used to avoid tracking partials
// and, here to make it work, we should had to track cepstrum peaks...
// (then this is useless)
class PartialsToFreqCepstrum
{
public:
    PartialsToFreqCepstrum(int bufferSize, BL_FLOAT sampleRate);
    
    virtual ~PartialsToFreqCepstrum();
    
    void Reset(BL_FLOAT sampleRate);
    
    BL_FLOAT ComputeFrequency(const vector<PartialTracker3::Partial> &partials);

protected:
    void PartialsToMagns(const vector<PartialTracker3::Partial> &partials,
                         WDL_TypedBuf<BL_FLOAT> *magns);
    
    int mBufferSize;
    BL_FLOAT mSampleRate;
};

#endif /* defined(__BL_SASViewer__PartialsToFreqCepstrum__) */
