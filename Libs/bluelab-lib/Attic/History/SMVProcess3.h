//
//  SMVProcess3.h
//  BL-PitchShift
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_PitchShift__SMVProcess3__
#define __BL_PitchShift__SMVProcess3__

#include <deque>
using namespace std;

#include <SmoothAvgHistogram.h>
#include <CMA2Smoother.h>

#include "FftProcessObj16.h"


// From StereoWidthProcess3
// From StereoVizProcess3

class SMVVolRender3;
class SMVProcess3 : public MultichannelProcess
{
public:
    enum DisplayMode
    {
        SIMPLE = 0,
        SOURCE,
        SCANNER,
        SPECTROGRAM,
        SPECTROGRAM2
    };
    
    SMVProcess3(int bufferSize,
                BL_FLOAT overlapping, BL_FLOAT oversampling,
                BL_FLOAT sampleRate);
    
    virtual ~SMVProcess3();
    
    void Reset();
    
    void Reset(int overlapping, int oversampling, BL_FLOAT sampleRate);
    
    void ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                         const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer);
    
    // Get the result
#if 0
    void GetWidthValues(WDL_TypedBuf<BL_FLOAT> *xValues,
                        WDL_TypedBuf<BL_FLOAT> *yValues,
                        WDL_TypedBuf<BL_FLOAT> *outColorWeights);
#endif
    
    // Display mode
    void SetDisplayMode(enum DisplayMode mode);
    
    // Angular mode
    void SetAngularMode(bool flag);
    
    // Set stereo width
    // If recomputeAll is true, then recompute all the current data (costly)
    // To be used when transport is not playing
    void SetStereoWidth(BL_FLOAT stereoWidth, bool recomputeAll);
    
    void SetVolRender(SMVVolRender3 *volRender);
    
protected:
    void TemporalSmoothing(WDL_TypedBuf<BL_FLOAT> *sourceRs,
                           WDL_TypedBuf<BL_FLOAT> *sourceThetas);

    void SpatialSmoothing(WDL_TypedBuf<BL_FLOAT> *sourceRs,
                          WDL_TypedBuf<BL_FLOAT> *sourceThetas);
    
    void NormalizeRs(WDL_TypedBuf<BL_FLOAT> *sourceRs, bool discardClip);
    
    void ComputeColorWeightsMagns(WDL_TypedBuf<BL_FLOAT> *outWeights,
                                  const WDL_TypedBuf<BL_FLOAT> &magnsL,
                                  const WDL_TypedBuf<BL_FLOAT> &magnsR);
    
    void ComputeColorWeightsFreqs(WDL_TypedBuf<BL_FLOAT> *outWeights,
                                  const WDL_TypedBuf<BL_FLOAT> &freqs);
    
    int GetDisplayRefreshRate();

    //void DiscardUnselected(WDL_TypedBuf<BL_FLOAT> *magns0,
    //                       WDL_TypedBuf<BL_FLOAT> *magns1);

    //void DiscardUnselectedStereo(WDL_TypedBuf<BL_FLOAT> *magns0,
    //                             WDL_TypedBuf<BL_FLOAT> *magns1,
    //                             const WDL_TypedBuf<BL_FLOAT> &balances);
    
    void ComputeSpectroY(WDL_TypedBuf<BL_FLOAT> *yValues, int numValues);

    // Get the center of selected data from the RayCaster
    void GetSelectedData(WDL_TypedBuf<WDL_FFT_COMPLEX> ioFftSamples[2]);

    void ComputeResult(const WDL_TypedBuf<BL_FLOAT> magns[2],
                       const WDL_TypedBuf<BL_FLOAT> phases[2],
                       WDL_TypedBuf<BL_FLOAT> *widthValuesX,
                       WDL_TypedBuf<BL_FLOAT> *widthValuesY,
                       WDL_TypedBuf<BL_FLOAT> *colorWeights);
    
    void RecomputeResult();

    void ApplyStereoWidth(WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples[2]);
    
    
    int mBufferSize;
    BL_FLOAT mOverlapping;
    BL_FLOAT mOversampling;
    BL_FLOAT mSampleRate;
    
    //WDL_TypedBuf<BL_FLOAT> mWidthValuesX;
    //WDL_TypedBuf<BL_FLOAT> mWidthValuesY;
    //WDL_TypedBuf<BL_FLOAT> mColorWeights;
    
    // For smoothing (to improve the display)
    SmoothAvgHistogram *mSourceRsHisto;
    SmoothAvgHistogram *mSourceThetasHisto;
    
    // display mode
    enum DisplayMode mDisplayMode;
    
    // Angular mode
    bool mAngularMode;
    
    // Stereo width
    BL_FLOAT mStereoWidth;
    
    // Do not refresh display at each step
    // (too CPU costly, particularly when using oversalpling=32)
    long long int mProcessNum;
    
    // Choosing this value as the same as overlapping is good
    int mDisplayRefreshRate;
    
    // Vol Render
    SMVVolRender3 *mVolRender;
    
    deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > mBufs[2];
};

#endif /* defined(__BL_PitchShift__StereoPhasesProcess__) */
