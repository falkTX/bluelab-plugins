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
//  SASViewerProcess3.h
//  BL-Waves
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_SASViewer__SASViewerProcess3__
#define __BL_SASViewer__SASViewerProcess3__

#ifdef IGRAPHICS_NANOVG

#include <deque>
using namespace std;

#include <SmoothAvgHistogram.h>
#include <CMA2Smoother.h>
#include <LinesRender2.h>
#include <PartialTracker5.h>
#include <SASFrame4.h>
#include <BlaTimer.h>
#include <FftProcessObj16.h>

class PartialTracker5;
class SASFrame4;
class SASViewerRender3;
class PhasesEstimPrusa;
class SASViewerProcess3 : public ProcessObj
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
    
    SASViewerProcess3(int bufferSize,
                     BL_FLOAT overlapping, BL_FLOAT oversampling,
                     BL_FLOAT sampleRate);
    
    virtual ~SASViewerProcess3();
    
    void Reset();
    
    void Reset(int bufferSize, int overlapping,
               int oversampling, BL_FLOAT sampleRate);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer) override;
    
    void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                              WDL_TypedBuf<BL_FLOAT> *scBuffer) override;
    
    // Use this to synthetize directly samples from partials
    void ProcessSamplesPost(WDL_TypedBuf<BL_FLOAT> *ioBuffer) override;
    
    void SetSASViewerRender(SASViewerRender3 *sasViewerRender);
    
    void SetMode(Mode mode);
    
    void SetShowTrackingLines(bool flag);
    
    void SetThreshold(BL_FLOAT threshold);
    
    void SetAmpFactor(BL_FLOAT factor);
    void SetFreqFactor(BL_FLOAT factor);
    void SetColorFactor(BL_FLOAT factor);
    void SetWarpingFactor(BL_FLOAT factor);
    
    void SetSynthMode(SASFrame4::SynthMode mode);
    void SetSynthEvenPartials(bool flag);
    void SetSynthOddPartials(bool flag);
    
    void SetHarmoNoiseMix(BL_FLOAT mix);
    
    void SetTimeSmoothCoeff(BL_FLOAT coeff);
    
    // Debug
    void DBG_SetDbgParam(BL_FLOAT param);

    void DBG_SetDebugPartials(bool flag);
    
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
    SASViewerRender3 *mSASViewerRender;
    
    PartialTracker5 *mPartialTracker;
    
    SASFrame4 *mSASFrame;
    
    BL_FLOAT mThreshold;

    BL_FLOAT mHarmoNoiseMix;
    
    // For tracking display
    deque<vector<LinesRender2::Point> > mPartialsPoints;
    
    // Keep an history, to avoid recomputing the whole lines each time
    // With this, we compute only the new extremity of the line
    vector<LinesRender2::Line> mPartialLines;
    
    //
    bool mShowTrackingLines;

    long int mAddNum;
    bool mSkipAdd;

    bool mDebugPartials;

    PhasesEstimPrusa *mPhasesEstim;
    
private:
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf5;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_SASViewer__SASViewerProcess3__) */
