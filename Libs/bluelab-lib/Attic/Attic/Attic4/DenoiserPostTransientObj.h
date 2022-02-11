//
//  DenoiserPostTransientObj.h
//  BL-Denoiser
//
//  Created by applematuer on 6/25/20.
//
//

#ifndef __BL_Denoiser__DenoiserPostTransientObj__
#define __BL_Denoiser__DenoiserPostTransientObj__

#include <PostTransientFftObj3.h>

// DenoiserPostTransientObj
// Use only 2 TransientShaperFftObj3 instead of 4
// We use one for left channel, and one for right channel.
// We use the same for signal L and noise L.
// We use the same for signal R and noise R.
class DenoiserPostTransientObj : public PostTransientFftObj3
{
public:
    DenoiserPostTransientObj(const vector<ProcessObj *> &processObjs,
                             int numChannels, int numScInputs,
                             int bufferSize, int overlapping, int freqRes,
                             BL_FLOAT sampleRate,
                             BL_FLOAT freqAmpRatio = -1.0,
                             BL_FLOAT transBoostFactor = -1.0);
    
    virtual ~DenoiserPostTransientObj();
    
protected:
    void ResultSamplesWinReady();
    
    // Override FftProcessObj16
    void ProcessAllFftSteps();
};

#endif /* defined(__BL_Denoiser__DenoiserPostTransientObj__) */
