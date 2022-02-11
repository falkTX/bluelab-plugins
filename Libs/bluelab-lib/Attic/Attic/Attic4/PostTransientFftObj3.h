//
//  PitchShiftTransientFftObj.h
//  BL-PitchShift
//
//  Created by Pan on 18/04/18.
//
//

#ifndef __BL_PitchShift__PostTransientFftObj3__
#define __BL_PitchShift__PostTransientFftObj3__

#include <vector>
using namespace std;

#include <FftProcessObj16.h>

// NOTE: can saturate if greater than 1.0

// OLD: before fix good separation "s"/"p" for Shaper
//#define TRANSIENT_BOOST_FACTOR 1.0

// NEW: after shaper fix
#define TRANSIENT_BOOST_FACTOR 2.0

// Above this threshold, we don't compute transients
// (performances gain)
#define TRANS_BOOST_EPS 1e-15


class TransientShaperFftObj3;

// PostTransientFftObj3: from PostTransientFftObj2
// - FftProcessObj16

class PostTransientFftObj3 : public FftProcessObj16
{
public:
    // Set skipFFT to true to skip fft and use only overlaping
    // Set numTransObjs to -1 to have the same number of TransientShaperFftObj3 than
    // the number of input channels
    PostTransientFftObj3(const vector<ProcessObj *> &processObjs,
                         int numChannels, int numScInputs,
                         int bufferSize, int overlapping, int freqRes,
                         BL_FLOAT sampleRate,
                         int numTransObjs = -1,
                         BL_FLOAT freqAmpRatio = -1.0,
                         BL_FLOAT transientBoostFactor = -1.0);
    
    virtual ~PostTransientFftObj3();
    
    void Reset();
    
    void Reset(int bufferSize, int overlapping, int freqRes, BL_FLOAT sampleRate);
    
    // Set the synthesis energy of the inner object only
    void SetKeepSynthesisEnergy(int channelNum, bool flag);

    void SetTransBoost(BL_FLOAT factor);
    
    void ResultSamplesWinReady();
    
protected:
    int GetNumChannels();
    
    TransientShaperFftObj3 *GetTransObj(int channelNum);
    
    void AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &inputs,
                    const vector<WDL_TypedBuf<BL_FLOAT> > &scInputs);
    
    BL_FLOAT mTransBoost;
    
    //DENOISER_OPTIM10
    int mNumChannelsTransient;
    
    //DENOISER_OPTIM11
    BL_FLOAT mTransientBoostFactor;
};

#endif /* defined(__BL_PitchShift__PostTransientFftObj3__) */
