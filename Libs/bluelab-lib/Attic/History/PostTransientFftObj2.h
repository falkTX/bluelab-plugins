//
//  PitchShiftTransientFftObj.h
//  BL-PitchShift
//
//  Created by Pan on 18/04/18.
//
//

#ifndef __BL_PitchShift__PostTransientFftObj2__
#define __BL_PitchShift__PostTransientFftObj2__

#include <vector>
using namespace std;

#include <FftProcessObj15.h>

class TransientShaperFftObj3;


class PostTransientFftObj2 : public FftProcessObj15
{
public:
    // Set skipFFT to true to skip fft and use only overlaping
    PostTransientFftObj2(const vector<ProcessObj *> &processObjs,
                         int numChannels, int numScInputs,
                         int bufferSize, int overlapping, int freqRes,
                         BL_FLOAT sampleRate);
    
    virtual ~PostTransientFftObj2();
    
    void Reset();
    
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
};

#endif /* defined(__BL_PitchShift__PostTransientFftObj2__) */
