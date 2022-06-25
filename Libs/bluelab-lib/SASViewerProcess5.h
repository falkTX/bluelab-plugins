/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
//
//  SASViewerProcess5.h
//  BL-Waves
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_SASViewer__SASViewerProcess5__
#define __BL_SASViewer__SASViewerProcess5__

#ifdef IGRAPHICS_NANOVG

#include <deque>
using namespace std;

#include <SmoothAvgHistogram.h>
#include <CMA2Smoother.h>
#include <LinesRender2.h>

#include <Partial.h>

#include <SASFrame6.h>
#include <SASFrameSynth.h>

#include <BlaTimer.h>
#include <FftProcessObj16.h>

class PartialTracker7;
class SASViewerRender5;
class SASFrameAna;
class SASFrameSynth;
class SASViewerProcess5 : public ProcessObj
{
public:
    // Display mode
    enum DisplayMode
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
    
    SASViewerProcess5(int bufferSize,
                      BL_FLOAT overlapping, BL_FLOAT oversampling,
                      BL_FLOAT sampleRate);
    
    virtual ~SASViewerProcess5();
    
    void Reset();
    
    void Reset(int bufferSize, int overlapping,
               int oversampling, BL_FLOAT sampleRate);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer) override;
    
    // Use this to synthetize directly samples from partials
    void ProcessSamplesPost(WDL_TypedBuf<BL_FLOAT> *ioBuffer) override;
    
    // Display
    //
    void SetSASViewerRender(SASViewerRender5 *sasViewerRender);
    
    void SetDisplayMode(DisplayMode mode);
    
    void SetShowTrackingLines(bool flag);
    void SetShowDetectionPoints(bool flag);
    
    void SetThreshold(BL_FLOAT threshold);
    void SetThreshold2(BL_FLOAT threshold2);
    
    void SetAmpFactor(BL_FLOAT factor);
    void SetFreqFactor(BL_FLOAT factor);
    void SetColorFactor(BL_FLOAT factor);
    void SetWarpingFactor(BL_FLOAT factor);

    // Parameters
    //
    void SetSynthMode(SASFrameSynth::SynthMode mode);
    void SetSynthEvenPartials(bool flag);
    void SetSynthOddPartials(bool flag);
    
    void SetHarmoNoiseMix(BL_FLOAT mix);
    
    void SetTimeSmoothCoeff(BL_FLOAT coeff);
    
    void SetTimeSmoothNoiseCoeff(BL_FLOAT coeff);

    void SetNeriDelta(BL_FLOAT delta);

    // Debug
    void DBG_SetDebugPartials(bool flag);
    
protected:
    void Display();
    
    void IdToColor(int idx, unsigned char color[3]);

#if 0
    // Optimized version
    void CreateLinesPrev(const vector<LinesRender2::Point> &prevPoints);
#endif
    // More optimized
    void CreateLines(const vector<LinesRender2::Point> &prevPoints);
    
    void DenormPartials(vector<Partial> *partials);

    
    // Display
    void DisplayDetection();
    void DisplayDetectionBeta0(bool addData); // Variation
    void DisplayZombiePoints();
    
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

    void PointsToLinesMix(const deque<vector<LinesRender2::Point> > &points0,
                          const deque<vector<LinesRender2::Point> > &points1,
                          vector<LinesRender2::Line> *lines);
    
    //
    WDL_TypedBuf<BL_FLOAT> mCurrentMagns;
    // Not filtered
    vector<Partial> mCurrentRawPartials;
    // Filtered
    vector<Partial> mCurrentNormPartials;
    
    // Renderer
    SASViewerRender5 *mSASViewerRender;
    
    PartialTracker7 *mPartialTracker;
    
    SASFrameAna *mSASFrameAna;
    SASFrameSynth *mSASFrameSynth;
    // Current SAS frame
    SASFrame6 mSASFrame;
    
    BL_FLOAT mThreshold;

    // For tracking detection
    deque<vector<LinesRender2::Point> > mPartialsPoints;

    // For displaying beta0
    deque<vector<vector<LinesRender2::Point> > > mPartialsSegments;

    // For zombie points
    deque<vector<LinesRender2::Point> > mPartialsPointsZombie;
    
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
    //WDL_TypedBuf<BL_FLOAT> mTmpBuf5;
    vector<LinesRender2::Line> mTmpBuf6;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf7;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf8;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf9;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf10;
    //WDL_TypedBuf<BL_FLOAT> mTmpBuf11;
    //WDL_TypedBuf<BL_FLOAT> mTmpBuf12;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf13;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf14;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf15;
    vector<LinesRender2::Line> mTmpBuf16;
    vector<Partial> mTmpBuf17;
    vector<LinesRender2::Point> mTmpBuf18;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_SASViewer__SASViewerProcess5__) */
