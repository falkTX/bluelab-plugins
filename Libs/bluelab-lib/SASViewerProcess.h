//
//  WavesProcess.h
//  BL-Waves
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_SASViewer__SASViewerProcess__
#define __BL_SASViewer__SASViewerProcess__

#ifdef IGRAPHICS_NANOVG

#include <deque>
using namespace std;

#include <SmoothAvgHistogram.h>
#include <CMA2Smoother.h>
#include <LinesRender2.h>
#include <PartialTracker3.h>
#include <SASFrame2.h>
#include <BlaTimer.h>

#include "FftProcessObj16.h"


#define SAS_VIEWER_PROCESS_PROFILE 0 //1 //0

// Debug
#define DEBUG_PARTIAL_TRACKING     0 //1 //0
// Hide points, and do not output sound after this freq
#define DEBUG_MAX_PARTIAL_FREQ 44100.0 //3000.0

class PartialTracker3;
class SASFrame2;

class SASViewerRender;
class SASViewerProcess : public ProcessObj
{
public:
    enum Mode
    {
        TRACKING,
        AMPLITUDE,
        FREQUENCY,
        COLOR,
        WARPING
    };
    
    SASViewerProcess(int bufferSize,
                     BL_FLOAT overlapping, BL_FLOAT oversampling,
                     BL_FLOAT sampleRate);
    
    virtual ~SASViewerProcess();
    
    void Reset();
    
    void Reset(int overlapping, int oversampling, BL_FLOAT sampleRate);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer) override;
    
    void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                              WDL_TypedBuf<BL_FLOAT> *scBuffer) override;
    
    // Use this to synthetize directly 1/4 of the samples from partials
    // (without overlap in internal)
    void ProcessSamplesBufferWin(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                 const WDL_TypedBuf<BL_FLOAT> *scBuffer) override;
    
    void SetSASViewerRender(SASViewerRender *sasViewerRender);
    
    void SetMode(Mode mode);
    
    void SetThreshold(BL_FLOAT threshold);
    
    void SetPitch(BL_FLOAT pitch);
    
    void SetNoiseMix(BL_FLOAT mix);
    
    void SetHarmonicSoundFlag(bool flag);
    
    void SetSynthMode(SASFrame2::SynthMode mode);
    
    // SideChain
    void SetUseSideChainFlag(bool flag);
    void SetScThreshold(BL_FLOAT threshold);
    void SetScHarmonicSoundFlag(bool flag);
    
    void SetMix(BL_FLOAT mix);

    void SetMixFreqFlag(bool flag);
    void SetMixNoiseFlag(bool flag);
    
#if 0
    static BL_FLOAT IdToFreq(int idx, BL_FLOAT sampleRate, int bufferSize);
    
    static int FreqToId(BL_FLOAT freq, BL_FLOAT sampleRate, int bufferSize);
#endif
    
    // Amp to dB
    static BL_FLOAT AmpToDBNorm(BL_FLOAT val);
    static BL_FLOAT DBToAmpNorm(BL_FLOAT val);
    
protected:
    void Display();
    
    void ScaleFreqs(WDL_TypedBuf<BL_FLOAT> *values);
    void AmpsToDb(WDL_TypedBuf<BL_FLOAT> *magns);
    
    // Apply freq scale to freq id
    int ScaleFreq(int idx);
    
    void DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                        const WDL_TypedBuf<BL_FLOAT> &phases);
    
    void DetectScPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                          const WDL_TypedBuf<BL_FLOAT> &phases);
    
    void AmpsToDBNorm(WDL_TypedBuf<BL_FLOAT> *amps);
    
    void IdToColor(int idx, unsigned char color[3]);
    
    void PartialToColor(const PartialTracker3::Partial &partial,
                        unsigned char color[4]);

    // Utils
    int FindIndex(const vector<int> &ids, int idx);
 
    int FindIndex(const vector<LinesRender2::Point> &points, int idx);

    // Original version (slow)
    void CreateLines(vector<vector<LinesRender2::Point> > *partialLines);
    
    // Optimized version
    // (optim x3)
    void CreateLines2(const vector<LinesRender2::Point> &prevPoints);
    
    // Mix noise and harmonic sound
    void MixHarmoNoise(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                       const WDL_TypedBuf<BL_FLOAT> &harmoBuffer);

    void MixFrames(SASFrame2 *result,
                   const SASFrame2 &frame0,
                   const SASFrame2 &frame1,
                   BL_FLOAT t);

    
    // Display
    void DisplayTracking();
    void DisplayAmplitude();
    void DisplayFrequency();
    void DisplayColor();
    void DisplayWarping();
    
    //int mBufferSize;
    //BL_FLOAT mOverlapping;
    //BL_FLOAT mOversampling;
    //BL_FLOAT mSampleRate;
    
    //WDL_TypedBuf<BL_FLOAT> mValues;
    
    WDL_TypedBuf<BL_FLOAT> mCurrentMagns;
    vector<PartialTracker3::Partial> mCurrentPartials;
    
    // Renderer
    SASViewerRender *mSASViewerRender;
    
    PartialTracker3 *mPartialTracker;
    PartialTracker3 *mScPartialTracker;
    
    SASFrame2 *mSASFrame;
    SASFrame2 *mScSASFrame;
    SASFrame2 *mMixSASFrame;
    
    Mode mMode;
    BL_FLOAT mThreshold;
    bool mHarmonicFlag;
    
    BL_FLOAT mNoiseMix;
    
    // SideChain
    bool mUseSideChain;
    BL_FLOAT mScThreshold;
    bool mScHarmonicFlag;
    bool mSideChainProvided;
    
    BL_FLOAT mMix;
    
    bool mMixFreqFlag;
    bool mMixNoiseFlag;
    
    // For tracking display
    deque<vector<LinesRender2::Point> > mPartialsPoints;
    long int mAddNum;
    
    // Keep an history, to avoid recomputing the whole lines each time
    // With this, we compute only the new extremity of the line
    vector<LinesRender2::Line> mPartialLines;
    
#if SAS_VIEWER_PROCESS_PROFILE
    BlaTimer mTimer0;
    long mTimerCount0;
    
    BlaTimer mTimer1;
    long mTimerCount1;
    
    BlaTimer mTimer2;
    long mTimerCount2;
    
    BlaTimer mTimer3;
    long mTimerCount3;
#endif
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_SASViewer__SASViewerProcess__) */
