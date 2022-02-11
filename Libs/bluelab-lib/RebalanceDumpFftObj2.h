//
//  RebalanceDumpFftObj2.hpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/14/20.
//
//

#ifndef RebalanceDumpFftObj2_hpp
#define RebalanceDumpFftObj2_hpp

#include <deque>
using namespace std;

#include <FftProcessObj16.h>

#include <Rebalance_defs.h>

// RebalanceDumpFftObj2: from RebalanceDumpFftObj
// for ResampProcessObj
class MelScale;
class Scale;
class RebalanceDumpFftObj2 : public MultichannelProcess
{
public:
    RebalanceDumpFftObj2(int bufferSize, BL_FLOAT sampleRate,
                         int numInputCols, int dumpOverlap = 1);
    
    virtual ~RebalanceDumpFftObj2();
    
    void
    ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                    const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer) override;
    
    bool HasEnoughData();
    
    // Mono spectrogram
    void GetSpectrogramData(WDL_TypedBuf<BL_FLOAT> cols[REBALANCE_NUM_SPECTRO_COLS]);
    // Stereo width "spectrogram"
    void GetStereoData(WDL_TypedBuf<BL_FLOAT> cols[REBALANCE_NUM_SPECTRO_COLS]);
    
protected:
    // Spectrogram
    void
    ProcessSpectrogramData(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                           const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer);

    // Stereo data
    void ProcessStereoData(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                           const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer);

    //
    int mNumInputCols;
    int mDumpOverlap;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mSpectroCols;
    deque<WDL_TypedBuf<BL_FLOAT> > mStereoCols;
    
    BL_FLOAT mSampleRate;

    // First filter method
    MelScale *mMelScale;

    // Second filter method
    Scale *mScale;
};

#endif /* RebalanceDumpFftObj2_hpp */
