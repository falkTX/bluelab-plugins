//
//  SMVProcess4.h
//  BL-PitchShift
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_PitchShift__SMVProcess4__
#define __BL_PitchShift__SMVProcess4__

#include <deque>
using namespace std;

#include <SmoothAvgHistogram.h>
#include <CMA2Smoother.h>

#include <FftProcessObj16.h>

#include <SMVProcessXComputer.h>
#include <SMVProcessYComputer.h>
#include <SMVProcessColComputer.h>


// From StereoWidthProcess3
// From StereoVizProcess3

// From SMVProcess3
// - implement the "algorithm" pattern, with combination of several objects
//
class SMVVolRender3;
class PhasesUnwrapper;
class TimeAxis3D;
class Axis3DFactory2;
class CMA2Smoother;
class SMVProcess4 : public MultichannelProcess
{
public:
    enum ModeX
    {
        MODE_X_SCOPE = 0,
        MODE_X_SCOPE_FLAT,
        //MODE_X_DIFF,
        
        //MODE_X_DIFF_FLAT,
        MODE_X_LISSAJOUS,
        MODE_X_EXP,
        
        MODE_X_FREQ,
        MODE_X_CHROMA,
        
        MODE_X_NUM_MODES
    };
    
    enum ModeY
    {
        MODE_Y_MAGN = 0,
        MODE_Y_FREQ,
        MODE_Y_CHROMA,
        
        MODE_Y_PHASES_FREQ,
        MODE_Y_PHASES_TIME,
        
        MODE_Y_NUM_MODES
    };
    
    enum ModeColor
    {
        MODE_COLOR_MAGN = 0,
        MODE_COLOR_FREQ,
        MODE_COLOR_CHROMA,
        
        MODE_COL_PHASES_FREQ,
        MODE_COL_PHASES_TIME,
        
        MODE_COL_PAN,
        MODE_COL_MS,
        
        MODE_COLOR_NUM_MODES
    };
    
    // For play button
    enum PlayMode
    {
        BYPASS,
        PLAY,
        RECORD
    };
    
    SMVProcess4(int bufferSize,
                BL_FLOAT overlapping, BL_FLOAT oversampling,
                BL_FLOAT sampleRate);
    
    virtual ~SMVProcess4();
    
    void Reset() override;
    
    void Reset(int overlapping, int oversampling, BL_FLOAT sampleRate);
    
    // For play button
    void SetPlayMode(PlayMode mode);
    
    void ProcessInputSamples(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                             const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer);

    
    void ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                         const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer);
    
    // Modes
    void SetModeX(enum ModeX mode);
    void SetModeY(enum ModeY mode);
    void SetModeColor(enum ModeColor mode);
    
    // Angular mode
    void SetAngularMode(bool flag);
    
    // Set stereo width
    // If recomputeAll is true, then recompute all the current data (costly)
    // To be used when transport is not playing
    void SetStereoWidth(BL_FLOAT stereoWidth, bool hostIsPlaying);
    
    void SetVolRender(SMVVolRender3 *volRender);
    
    // Play
    void SetIsPlaying(bool flag);
    void SetHostIsPlaying(bool flag);
    
    // Time axis
    //
    
    // When number of slices changes
    void ResetTimeAxis();
    
    // When transport is playing
    // Called from plug.
    void UpdateTimeAxis(BL_FLOAT currentTime);
    
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
    //
    // extractSliceHistory: extract slice form history,
    // or use the input fft samples ?
    void GetSelectedData(WDL_TypedBuf<WDL_FFT_COMPLEX> ioFftSamples[2],
                         long sliceNum, bool extractSliceHistory = true);

    void ComputeResult(const WDL_TypedBuf<BL_FLOAT> samples[2],
                       const WDL_TypedBuf<BL_FLOAT> magns[2],
                       const WDL_TypedBuf<BL_FLOAT> phases[2],
                       WDL_TypedBuf<BL_FLOAT> *widthValuesX,
                       WDL_TypedBuf<BL_FLOAT> *widthValuesY,
                       WDL_TypedBuf<BL_FLOAT> *colorWeights);
    
    void RecomputeResult();

    // for fft samples
    void ApplyStereoWidth(WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples[2]);
    
    // For samples
    void ApplyStereoWidth(WDL_TypedBuf<BL_FLOAT> samples[2]);

    // Axes
    void SetupAxis(Axis3D *axis);
    void CreateAxes();

    bool IsXPolarNotScalable();

    //
    void InitTimeAxis();

    
    //
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
    deque<WDL_TypedBuf<BL_FLOAT> > mSamplesBufs[2];
    
    // Modes
    enum ModeX mModeX;
    enum ModeY mModeY;
    enum ModeColor mModeColor;
    
    // Computers
    SMVProcessXComputer *mXComputers[MODE_X_NUM_MODES];
    SMVProcessYComputer *mYComputers[MODE_Y_NUM_MODES];
    SMVProcessColComputer *mColComputers[MODE_COLOR_NUM_MODES];
    
    PhasesUnwrapper *mPhasesUnwrapper;
    
    // Keep the original samples
    WDL_TypedBuf<BL_FLOAT> mCurrentSamples[2];
    
    // Play
    //
    PlayMode mPlayMode;
    
    long mCurrentPlayLine;
    
    // Play button
    bool mIsPlaying;
    
    // Host play
    bool mHostIsPlaying;
    
    TimeAxis3D *mTimeAxis;
    BL_FLOAT mPrevTime;
    
    //
    Axis3DFactory2 *mAxisFactory;

    //
    CMA2Smoother *mSmoother;
};

#endif /* defined(__BL_PitchShift__StereoPhasesProcess__) */
