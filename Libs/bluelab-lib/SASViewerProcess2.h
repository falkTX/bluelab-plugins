//
//  SASViewerProcess2.h
//  BL-Waves
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_SASViewer__SASViewerProcess2__
#define __BL_SASViewer__SASViewerProcess2__

#ifdef IGRAPHICS_NANOVG

#include <deque>
using namespace std;

#include <SmoothAvgHistogram.h>
#include <CMA2Smoother.h>
#include <LinesRender2.h>
#include <PartialTracker5.h>
#include <SASFrame3.h>
#include <BlaTimer.h>
#include <FftProcessObj16.h>

class PartialTracker5;
class SASFrame3;
class SASViewerRender2;
class SASViewerProcess2 : public ProcessObj
{
public:
    enum Mode
    {
        TRACKING = 0,
        HARMO,
        NOISE,
        AMPLITUDE,
        FREQUENCY,
        COLOR,
        WARPING,
        
        NUM_MODES
    };
    
    SASViewerProcess2(int bufferSize,
                     BL_FLOAT overlapping, BL_FLOAT oversampling,
                     BL_FLOAT sampleRate);
    
    virtual ~SASViewerProcess2();
    
    void Reset();
    
    void Reset(int overlapping, int oversampling, BL_FLOAT sampleRate);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer) override;
    
    void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                              WDL_TypedBuf<BL_FLOAT> *scBuffer) override;
    
    // Use this to synthetize directly samples from partials
    void ProcessSamplesPost(WDL_TypedBuf<BL_FLOAT> *ioBuffer) override;
    
    void SetSASViewerRender(SASViewerRender2 *sasViewerRender);
    
    void SetMode(Mode mode);
    
    void SetShowTrackingLines(bool flag);
    
    void SetThreshold(BL_FLOAT threshold);
    
    void SetPitch(BL_FLOAT pitch);
    
    void SetHarmonicSoundFlag(bool flag);
    
    void SetSynthMode(SASFrame3::SynthMode mode);

    void SetEnableOutHarmo(bool flag);
    void SetEnableOutNoise(bool flag);
    
    void SetTimeSmoothCoeff(BL_FLOAT coeff);
    
    // Debug
    void DBG_SetDbgParam(BL_FLOAT param);
    
    void SetTimeSmoothNoiseCoeff(BL_FLOAT coeff);
    
protected:
    void Display();
    
    // Apply freq scale to freq id
    int ScaleFreq(int idx);
    
    void IdToColor(int idx, unsigned char color[3]);
    
    void PartialToColor(const PartialTracker5::Partial &partial,
                        unsigned char color[4]);

    // Utils
    int FindIndex(const vector<int> &ids, int idx);
 
    int FindIndex(const vector<LinesRender2::Point> &points, int idx);
    
    // Optimized version
    void CreateLines(const vector<LinesRender2::Point> &prevPoints);

    void DenormPartials(vector<PartialTracker5::Partial> *partials);

    
    // Display
    void DisplayTracking();
    void DisplayHarmo();
    void DisplayNoise();
    void DisplayAmplitude();
    void DisplayFrequency();
    void DisplayColor();
    void DisplayWarping();
    
    //
    //int mBufferSize;
    //BL_FLOAT mOverlapping;
    //BL_FLOAT mOversampling;
    //BL_FLOAT mSampleRate;
    
    //
    WDL_TypedBuf<BL_FLOAT> mCurrentMagns;
    vector<PartialTracker5::Partial> mCurrentNormPartials;
    
    // Renderer
    SASViewerRender2 *mSASViewerRender;
    
    PartialTracker5 *mPartialTracker;
    
    SASFrame3 *mSASFrame;
    
    Mode mMode;
    BL_FLOAT mThreshold;
    bool mHarmonicFlag;
    
    bool mEnableOutHarmo;
    bool mEnableOutNoise;
    
    // For tracking display
    deque<vector<LinesRender2::Point> > mPartialsPoints;
    long int mAddNum;
    
    // Keep an history, to avoid recomputing the whole lines each time
    // With this, we compute only the new extremity of the line
    vector<LinesRender2::Line> mPartialLines;
    
    //
    bool mShowTrackingLines;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_SASViewer__SASViewerProcess2__) */
