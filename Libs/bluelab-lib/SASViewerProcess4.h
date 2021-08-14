//
//  SASViewerProcess4.h
//  BL-Waves
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_SASViewer__SASViewerProcess4__
#define __BL_SASViewer__SASViewerProcess4__

#ifdef IGRAPHICS_NANOVG

#include <deque>
using namespace std;

#include <SmoothAvgHistogram.h>
#include <CMA2Smoother.h>
#include <LinesRender2.h>
#include <SASFrame5.h>
#include <Partial.h>
#include <BlaTimer.h>
#include <FftProcessObj16.h>

class PartialTracker6;
class SASFrame5;
class SASViewerRender4;
class PhasesEstimPrusa;
class SASViewerProcess4 : public ProcessObj
{
public:
    enum Mode
    {
        DETECTION = 0,
        TRACKING,
        HARMO,
        NOISE,
        AMPLITUDE,
        FREQUENCY,
        COLOR,
        WARPING,
        
        NUM_MODES
    };
    
    SASViewerProcess4(int bufferSize,
                     BL_FLOAT overlapping, BL_FLOAT oversampling,
                     BL_FLOAT sampleRate);
    
    virtual ~SASViewerProcess4();
    
    void Reset();
    
    void Reset(int bufferSize, int overlapping,
               int oversampling, BL_FLOAT sampleRate);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer) override;
    
    void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                              WDL_TypedBuf<BL_FLOAT> *scBuffer) override;
    
    // Use this to synthetize directly samples from partials
    void ProcessSamplesPost(WDL_TypedBuf<BL_FLOAT> *ioBuffer) override;
    
    void SetSASViewerRender(SASViewerRender4 *sasViewerRender);
    
    void SetMode(Mode mode);
    
    void SetShowTrackingLines(bool flag);
    void SetShowDetectionPoints(bool flag);
    
    void SetThreshold(BL_FLOAT threshold);
    
    void SetAmpFactor(BL_FLOAT factor);
    void SetFreqFactor(BL_FLOAT factor);
    void SetColorFactor(BL_FLOAT factor);
    void SetWarpingFactor(BL_FLOAT factor);
    
    void SetSynthMode(SASFrame5::SynthMode mode);
    void SetSynthEvenPartials(bool flag);
    void SetSynthOddPartials(bool flag);
    
    void SetHarmoNoiseMix(BL_FLOAT mix);
    
    void SetTimeSmoothCoeff(BL_FLOAT coeff);
    
    // Debug
    void DBG_SetDebugPartials(bool flag);
    
    void SetTimeSmoothNoiseCoeff(BL_FLOAT coeff);
    
protected:
    void Display();
    
    // Apply freq scale to freq id
    int ScaleFreq(int idx);
    
    void IdToColor(int idx, unsigned char color[3]);
    
    void PartialToColor(const Partial &partial, unsigned char color[4]);

    // Utils
    int FindIndex(const vector<int> &ids, int idx);
 
    int FindIndex(const vector<LinesRender2::Point> &points, int idx);
    
    // Optimized version
    void CreateLines(const vector<LinesRender2::Point> &prevPoints);

    void DenormPartials(vector<Partial> *partials);

    
    // Display
    void DisplayDetection();
    void DisplayDetectionBeta0(bool addData); // Variation
    void DisplayTracking();
    
    void DisplayHarmo();
    void DisplayNoise();
    void DisplayAmplitude();
    void DisplayFrequency();
    void DisplayColor();
    void DisplayWarping();

    void PointsToLines(const deque<vector<LinesRender2::Point> > &points,
                       vector<LinesRender2::Line> *lines);
    void SegmentsToLines(const deque<vector<vector<LinesRender2::Point> > >&segments,
                         vector<LinesRender2::Line> *lines);
    
    //
    WDL_TypedBuf<BL_FLOAT> mCurrentMagns;
    // Not filtered
    vector<Partial> mCurrentRawPartials;
    // Filtered
    vector<Partial> mCurrentNormPartials;
    
    // Renderer
    SASViewerRender4 *mSASViewerRender;
    
    PartialTracker6 *mPartialTracker;
    
    SASFrame5 *mSASFrame;
    
    BL_FLOAT mThreshold;

    BL_FLOAT mHarmoNoiseMix;

    // For tracking detection
    deque<vector<LinesRender2::Point> > mPartialsPoints;

    // For displaying beta0
    deque<vector<vector<LinesRender2::Point> > > mPartialsSegments;
    
    // For tracking display
    deque<vector<LinesRender2::Point> > mFilteredPartialsPoints;
    
    // Keep an history, to avoid recomputing the whole lines each time
    // With this, we compute only the new extremity of the line
    vector<LinesRender2::Line> mPartialLines;
    
    //
    bool mShowTrackingLines;
    bool mShowDetectionPoints;

    long int mAddNum;
    bool mSkipAdd;

    bool mDebugPartials;

    PhasesEstimPrusa *mPhasesEstim;

    // Data scale for viewing
    Scale *mViewScale;
    Scale::Type mViewXScale;
    Scale::FilterBankType mViewXScaleFB;
    
private:
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf5;
    vector<LinesRender2::Line> mTmpBuf6;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf7;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf8;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf9;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf10;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf11;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf12;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf13;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf14;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf15;
    vector<LinesRender2::Line> mTmpBuf16;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_SASViewer__SASViewerProcess4__) */
