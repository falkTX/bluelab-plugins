//
//  StereoVizProcess4.cpp
//  BL-PitchShift
//
//  Created by Pan on 20/04/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include <BLDebug.h>
#include <DebugGraph.h>

#include <SASViewerRender4.h>

#include <SASFrame5.h>

#include <PhasesEstimPrusa.h>

#include <PartialTracker6.h>
#include <QIFFT.h> // For empirical coeffs
#include "SASViewerProcess4.h"


#define SHOW_ONLY_ALIVE 0 //1
#define MIN_AGE_DISPLAY 0 //10 // 4

// 1: more smooth
// 0: should be more correct
#define DISPLAY_HARMO_SUBSTRACT 1

// Use full SASFrame
#define OUT_HARMO_SAS_FRAME 1 //0 // ORIGIN
// Use extracted harmonic envelope
#define OUT_HARMO_EXTRACTED_ENV 0 //1
// Use input partials (not modified by color etc.)
#define OUT_HARMO_INPUT_PARTIALS 0 //1

// Does not improve transients at all (result is identical)
#define USE_PRUSA_PHASES_ESTIM 0 //1

#define DBG_DISPLAY_BETA0 1
#define DBG_DISPLAY_ZOMBIES 1

// Bypass all processing (was for testing fft reconstruction with gaussian windows
#define DBG_BYPASS 0 //1 // 0

#define VIEW_ALPHA0_COEFF 1e4 //5e2
#define VIEW_BETA0_COEFF 1e3

#if !FIND_PEAK_COMPAT
#define VIEW_ALPHA0_COEFF 5.0/*50.0*//EMPIR_ALPHA0_COEFF // Red segments, for amps
#define VIEW_BETA0_COEFF 0.02/EMPIR_BETA0_COEFF // Blue segments, for freqs
#endif

// If not set to 1, we will skip steps, depending on the speed
// (for example we won't display every wombie point)
#define DISPLAY_EVERY_STEP 1 // 0

// BAD
// Set to 1 to try to time smooth on complex values later 
#define SET_PT_DATA_COMPLEX 0 //1

SASViewerProcess4::SASViewerProcess4(int bufferSize,
                                     BL_FLOAT overlapping, BL_FLOAT oversampling,
                                     BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    //mBufferSize = bufferSize;
    mOverlapping = overlapping;
    //mOversampling = oversampling;
    mFreqRes = oversampling;
    
    mSampleRate = sampleRate;
    
    mSASViewerRender = NULL;
    
    mPartialTracker = new PartialTracker6(bufferSize, sampleRate, overlapping);
        
    BL_FLOAT minAmpDB = mPartialTracker->GetMinAmpDB();
    
    mSASFrame = new SASFrame5(bufferSize, sampleRate,
                              overlapping, oversampling);
    mSASFrame->SetMinAmpDB(minAmpDB);
    
    mThreshold = -60.0;
    
    mHarmoNoiseMix = 1.0;
    
    // For additional lines
    mAddNum = 0;
    mSkipAdd = false;
    
    mShowTrackingLines = true;
    mShowDetectionPoints = true;

    mDebugPartials = false;

    mPhasesEstim = NULL;
#if USE_PRUSA_PHASES_ESTIM
    mPhasesEstim = new PhasesEstimPrusa(bufferSize, overlapping, 1, sampleRate);
#endif

    // Data scale
    mViewScale = new Scale();
    mViewXScale = Scale::MEL_FILTER;
    mViewXScaleFB = Scale::FILTER_BANK_MEL;
}

SASViewerProcess4::~SASViewerProcess4()
{
    delete mPartialTracker;
    delete mSASFrame;

#if USE_PRUSA_PHASES_ESTIM
    delete mPhasesEstim;
#endif

    delete mViewScale;
}

void
SASViewerProcess4::Reset()
{
    Reset(mBufferSize, mOverlapping, mFreqRes, mSampleRate);

    //mSASFrame->Reset(mSampleRate);

#if USE_PRUSA_PHASES_ESTIM
    mPhasesEstim->Reset();
#endif
}

void
SASViewerProcess4::Reset(int bufferSize, int overlapping,
                         int oversampling, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    
    mOverlapping = overlapping;
    mFreqRes = oversampling;
    
    mSampleRate = sampleRate;
    
    mSASFrame->Reset(bufferSize, overlapping, oversampling, sampleRate);

#if USE_PRUSA_PHASES_ESTIM
    mPhasesEstim->Reset(mBufferSize, mOverlapping, mFreqRes, mSampleRate);
#endif
}

void
SASViewerProcess4::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                    const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)

{
#if DBG_BYPASS
    return;
#endif
   
    WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples0 = mTmpBuf0;
    fftSamples0 = *ioBuffer;
    
    // Take half of the complexes
    WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples = mTmpBuf1;
    BLUtils::TakeHalf(fftSamples0, &fftSamples);

    // Need to compute magns and phases here for later mSASFrame->SetInputData()G
    WDL_TypedBuf<BL_FLOAT> &magns = mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> &phases = mTmpBuf3;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, fftSamples);
    
#if !SET_PT_DATA_COMPLEX
    mPartialTracker->SetData(magns, phases);
#else
    mPartialTracker->SetData(fftSamples);
#endif
    
    // DetectPartials
    mPartialTracker->DetectPartials();

    // Try to provide the first partials, even is they are not yet filtered
    mPartialTracker->GetPartialsRAW(&mCurrentRawPartials);
    
    mPartialTracker->ExtractNoiseEnvelope();

    mPartialTracker->FilterPartials();
        
    //
    mPartialTracker->GetPreProcessedMagns(&mCurrentMagns);
            
    //
    mSASFrame->SetInputData(magns, phases);
        
    // Silence
    BLUtils::FillAllZero(&magns);
    
    if (mPartialTracker != NULL)
    {
        vector<Partial> normPartials;
        mPartialTracker->GetPartials(&normPartials);

        mCurrentNormPartials = normPartials;
        
        vector<Partial> partials = normPartials;
        mPartialTracker->DenormPartials(&partials);
        
        mSASFrame->SetPartials(partials);

        BL_FLOAT noiseCoeff;
        BL_FLOAT harmoCoeff;
        BLUtils::MixParamToCoeffs(mHarmoNoiseMix, &noiseCoeff, &harmoCoeff);
        
        WDL_TypedBuf<BL_FLOAT> &noise = mTmpBuf4;
        mPartialTracker->GetNoiseEnvelope(&noise);
        
        mSASFrame->SetNoiseEnvelope(noise);
        
        mPartialTracker->DenormData(&noise);

        BLUtils::MultValues(&noise, noiseCoeff);
        
        // Noise!
        magns = noise;

#if USE_PRUSA_PHASES_ESTIM
        mPhasesEstim->Process(magns, &phases);
#endif
        
#if OUT_HARMO_EXTRACTED_ENV
        WDL_TypedBuf<BL_FLOAT> &harmo = mTmpBuf5;
        mPartialTracker->GetHarmonicEnvelope(&harmo);
        
        mPartialTracker->DenormData(&harmo);

        BLUtils::MultValues(&harmo, harmoCoeff);
        
        BLUtils::AddValues(&magns, harmo);
#endif
        
        Display();
    }
    
    // For noise envelope
    BLUtilsComp::MagnPhaseToComplex(&fftSamples, magns, phases);
    BLUtilsFft::FillSecondFftHalf(fftSamples, ioBuffer);
}

void
SASViewerProcess4::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                        WDL_TypedBuf<BL_FLOAT> *scBuffer)
{
#if DBG_BYPASS
    return;
#endif
    
    if (!mSASFrame->ComputeSamplesFlag())
        return;
    
    // Create a separate buffer for samples synthesis from partials
    // (because it doesn't use overlap)
    WDL_TypedBuf<BL_FLOAT> samplesBuffer;
    BLUtils::ResizeFillZeros(&samplesBuffer, ioBuffer->GetSize());

    BL_FLOAT noiseCoeff;
    BL_FLOAT harmoCoeff;
    BLUtils::MixParamToCoeffs(mHarmoNoiseMix, &noiseCoeff, &harmoCoeff);
        
#if OUT_HARMO_SAS_FRAME
    // Compute the samples from partials
    mSASFrame->ComputeSamplesResynth(&samplesBuffer);

    BLUtils::MultValues(&samplesBuffer, harmoCoeff);
    
    // ioBuffer may already contain noise
    BLUtils::AddValues(ioBuffer, samplesBuffer);
#endif
    
#if OUT_HARMO_INPUT_PARTIALS
    // Compute the samples from partials
    mSASFrame->ComputeSamples(&samplesBuffer);

    BLUtils::MultValues(&samplesBuffer, harmoCoeff);
    
    // ioBuffer may already contain noise
    BLUtils::AddValues(ioBuffer, samplesBuffer);
#endif
}

// Use this to synthetize directly the samples from partials
void
SASViewerProcess4::ProcessSamplesPost(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
#if DBG_BYPASS
    return;
#endif
    
    if (!mSASFrame->ComputeSamplesPostFlag())
        return;
    
    // Create a separate buffer for samples synthesis from partials
    // (because it doesn't use overlap)
    WDL_TypedBuf<BL_FLOAT> samplesBuffer;
    samplesBuffer.Resize(ioBuffer->GetSize());
    BLUtils::FillAllZero(&samplesBuffer);

    BL_FLOAT noiseCoeff;
    BL_FLOAT harmoCoeff;
    BLUtils::MixParamToCoeffs(mHarmoNoiseMix, &noiseCoeff, &harmoCoeff);
        
#if OUT_HARMO_SAS_FRAME
    // Compute the samples from partials
    mSASFrame->ComputeSamplesResynthPost(&samplesBuffer);

    BLUtils::MultValues(&samplesBuffer, harmoCoeff);
    
    // ioBuffer may already contain noise
    BLUtils::AddValues(ioBuffer, samplesBuffer);
#endif
    
#if OUT_HARMO_INPUT_PARTIALS
    // Compute the samples from partials
    mSASFrame->ComputeSamplesPost(&samplesBuffer);

    BLUtils::MultValues(&samplesBuffer, harmoCoeff);
    
    // ioBuffer may already contain noise
    BLUtils::AddValues(ioBuffer, samplesBuffer);
#endif
}

void
SASViewerProcess4::SetSASViewerRender(SASViewerRender4 *sasViewerRender)
{
    mSASViewerRender = sasViewerRender;
}

void
SASViewerProcess4::SetMode(Mode mode)
{
    if (mSASViewerRender != NULL)
        mSASViewerRender->SetMode(mode);
}

void
SASViewerProcess4::SetShowDetectionPoints(bool flag)
{
    mShowDetectionPoints = flag;
    
    if (mSASViewerRender != NULL)
        mSASViewerRender->ShowAdditionalPoints(DETECTION, flag);
}

void
SASViewerProcess4::SetShowTrackingLines(bool flag)
{
    mShowTrackingLines = flag;
    
    if (mSASViewerRender != NULL)
        mSASViewerRender->ShowAdditionalLines(TRACKING, flag);
}

void
SASViewerProcess4::SetThreshold(BL_FLOAT threshold)
{
    mThreshold = threshold;
    mPartialTracker->SetThreshold(threshold);
}

void
SASViewerProcess4::SetAmpFactor(BL_FLOAT factor)
{
    mSASFrame->SetAmpFactor(factor);
}

void
SASViewerProcess4::SetFreqFactor(BL_FLOAT factor)
{
    mSASFrame->SetFreqFactor(factor);
}

void
SASViewerProcess4::SetColorFactor(BL_FLOAT factor)
{
    mSASFrame->SetColorFactor(factor);
}

void
SASViewerProcess4::SetWarpingFactor(BL_FLOAT factor)
{
    mSASFrame->SetWarpingFactor(factor);
}

void
SASViewerProcess4::SetSynthMode(SASFrame5::SynthMode mode)
{
    mSASFrame->SetSynthMode(mode);
}

void
SASViewerProcess4::SetSynthEvenPartials(bool flag)
{
    mSASFrame->SetSynthEvenPartials(flag);
}

void
SASViewerProcess4::SetSynthOddPartials(bool flag)
{
    mSASFrame->SetSynthOddPartials(flag);
}

void
SASViewerProcess4::SetHarmoNoiseMix(BL_FLOAT mix)
{
    mHarmoNoiseMix = mix;
}

void
SASViewerProcess4::DBG_SetDebugPartials(bool flag)
{
    mDebugPartials = flag;
}

void
SASViewerProcess4::SetTimeSmoothCoeff(BL_FLOAT coeff)
{
    if (mPartialTracker != NULL)
        mPartialTracker->SetTimeSmoothCoeff(coeff);
}

void
SASViewerProcess4::SetTimeSmoothNoiseCoeff(BL_FLOAT coeff)
{
    if (mPartialTracker != NULL)
        mPartialTracker->SetTimeSmoothNoiseCoeff(coeff);
}

void
SASViewerProcess4::Display()
{
#if !DISPLAY_EVERY_STEP
    if (mSASViewerRender != NULL)
    {
        int speed = mSASViewerRender->GetSpeed();
        mSkipAdd = ((mAddNum++ % speed) != 0);
    }
    
    if (mSkipAdd)
        return;
#endif
    
#if DBG_DISPLAY_BETA0
    DisplayDetectionBeta0(false);
#endif
    
    DisplayDetection();

#if DBG_DISPLAY_ZOMBIES
    DisplayZombiePoints();
#endif
    
    DisplayTracking();
    
    DisplayHarmo();
    
    DisplayNoise();
    
    DisplayAmplitude();
    
    DisplayFrequency();
    
    DisplayColor();
    
    DisplayWarping();
}

void
SASViewerProcess4::IdToColor(int idx, unsigned char color[3])
{
    if (idx == -1)
    {
        color[0] = 255;
        color[1] = 0;
        color[2] = 0;
        
        return;
    }
    
    int r = (678678 + idx*12345)%255;
    int g = (3434345 + idx*123345435345)%255;
    int b = (997867 + idx*12345114222)%255;
    
    color[0] = r;
    color[1] = g;
    color[2] = b;
}

void
SASViewerProcess4::PartialToColor(const Partial &partial, unsigned char color[4])
{
    if (partial.mId == -1)
    {
        // Green
        color[0] = 0;
        color[1] = 255;
        color[2] = 0;
        color[3] = 255;
        
        return;
    }
    
    int deadAlpha = 255;
    
#if SHOW_ONLY_ALIVE
    deadAlpha = 0;
#endif
    
    if (partial.mState == Partial::ZOMBIE)
    {
        // Green
        color[0] = 255;
        color[1] = 0;
        color[2] = 255;
        color[3] = deadAlpha;
        
        return;
    }
    
    if (partial.mState == Partial::DEAD)
    {
        // Green
        color[0] = 255;
        color[1] = 0;
        color[2] = 0;
        color[3] = deadAlpha;
        
        return;
    }
    
    int alpha = 255;
    if (partial.mAge < MIN_AGE_DISPLAY)
    {
        alpha = 0;
    }
    
    IdToColor((int)partial.mId, color);
    color[3] = alpha;
}

void
SASViewerProcess4::DisplayDetection()
{
    if (mSASViewerRender != NULL)
    {
        mSASViewerRender->ShowAdditionalPoints(DETECTION, mShowDetectionPoints);

        WDL_TypedBuf<BL_FLOAT> &data = mTmpBuf7;
        mViewScale->ApplyScaleFilterBank(mViewXScaleFB, &data, mCurrentMagns,
                                         mSampleRate, mCurrentMagns.GetSize());
    
        mSASViewerRender->AddData(DETECTION, data);
        
        mSASViewerRender->SetLineMode(DETECTION, LinesRender2::LINES_FREQ);

        // Add points corresponding to raw detected partials
        vector<Partial> partials = mCurrentRawPartials;

        // Create line
        vector<LinesRender2::Point> line;
        for (int i = 0; i < partials.size(); i++)
        {
            const Partial &partial = partials[i];
            
            LinesRender2::Point p;
            
            BL_FLOAT partialX = partial.mFreq;

            partialX =
                mViewScale->ApplyScale(mViewXScale, partialX,
                                       (BL_FLOAT)0.0, (BL_FLOAT)(mSampleRate*0.5));
                                              
            p.mX = partialX - 0.5;
            p.mY = partial.mAmp;
            
            p.mZ = 1.0;
            
            p.mId = (int)partial.mId;
            
            line.push_back(p);
        }

        //
        int numSlices = mSASViewerRender->GetNumSlices();

        // Keep track of the points we pop
        vector<LinesRender2::Point> prevPoints;
        // Initialize, just in case
        if (!mPartialsPoints.empty())
            prevPoints = mPartialsPoints[0];
        
        mPartialsPoints.push_back(line);
        
        while(mPartialsPoints.size() > numSlices)
        {
            prevPoints = mPartialsPoints[0];
            mPartialsPoints.pop_front();
        }
        
        // It is cool like that: lite blue with alpha
        //unsigned char color[4] = { 64, 64, 255, 255 };
        
        // Green
        unsigned char color[4] = { 0, 255, 0, 255 };
        
        // Set color
        for (int j = 0; j < mPartialsPoints.size(); j++)
        {
            vector<LinesRender2::Point> &line2 = mPartialsPoints[j];

            for (int i = 0; i < line2.size(); i++)
            {
                LinesRender2::Point &p = line2[i];

                // Color
                p.mR = color[0];
                p.mG = color[1];
                p.mB = color[2];
                p.mA = color[3];
            }
        }

        // Update Z
        int divisor = mSASViewerRender->GetNumSlices();
        if (divisor <= 0)
            divisor = 1;
        BL_FLOAT incrZ = 1.0/divisor;
        for (int j = 0; j < mPartialsPoints.size(); j++)
        {
            vector<LinesRender2::Point> &line2 = mPartialsPoints[j];

            if (line2.empty())
                continue;
            
            BL_FLOAT z = line2[0].mZ;
            z -= incrZ;

            for (int i = 0; i < line2.size(); i++)
            {
                LinesRender2::Point &p = line2[i];
                p.mZ = z; 
            }
        }
        
        BL_FLOAT lineWidth = 4.0;

        vector<LinesRender2::Line> &partialLines = mTmpBuf6;
        PointsToLines(mPartialsPoints, &partialLines);
        
        mSASViewerRender->SetAdditionalPoints(DETECTION, partialLines, lineWidth);
    }
}

// Set add data to false if we already call DisplayDetection()
void
SASViewerProcess4::DisplayDetectionBeta0(bool addData)
{
    if (mSASViewerRender != NULL)
    {
        mSASViewerRender->ShowAdditionalLines(DETECTION, mShowDetectionPoints);

        WDL_TypedBuf<BL_FLOAT> &data = mTmpBuf15;
        mViewScale->ApplyScaleFilterBank(mViewXScaleFB, &data, mCurrentMagns,
                                         mSampleRate, mCurrentMagns.GetSize());

        if (addData)
            mSASViewerRender->AddData(DETECTION, data);
        
        mSASViewerRender->SetLineMode(DETECTION, LinesRender2::LINES_FREQ);

        // Add points corresponding to raw detected partials
        vector<Partial> partials = mCurrentNormPartials; //mCurrentRawPartials;

        // Create line
        vector<vector<LinesRender2::Point> > segments;
        for (int i = 0; i < partials.size(); i++)
        {
            vector<LinesRender2::Point> lineAlpha;
            vector<LinesRender2::Point> lineBeta;
                
            const Partial &partial = partials[i];
            
            // First point (standard)
            LinesRender2::Point p0;
            
            BL_FLOAT partialX0 = partial.mFreq;
            partialX0 =
                mViewScale->ApplyScale(mViewXScale, partialX0,
                                       (BL_FLOAT)0.0, (BL_FLOAT)(mSampleRate*0.5));
                                              
            p0.mX = partialX0 - 0.5;
            p0.mY = partial.mAmp;
            p0.mZ = 1.0;
            p0.mId = (int)partial.mId;

            lineAlpha.push_back(p0);
            lineBeta.push_back(p0);

            // Second point (extrapolated)
            LinesRender2::Point p1;

            // Add QIFFT alpha using correct scale
            BL_FLOAT ampQIFFT =
                mPartialTracker->PartialScaleToQIFFTScale(partial.mAmp);
            
            ampQIFFT += partial.mAlpha0*VIEW_ALPHA0_COEFF;

            BL_FLOAT ampDbNorm =
                mPartialTracker->QIFFTScaleToPartialScale(ampQIFFT);
            BL_FLOAT partialY1 = ampDbNorm;
            
            p1.mX = partialX0 - 0.5;
            p1.mY = partialY1;
            p1.mZ = 1.0;
            p1.mId = (int)partial.mId;

            // Red
            p1.mR = 255;
            p1.mG = 0;
            p1.mB = 0;
            p1.mA = 255;
            
            lineAlpha.push_back(p1);
            segments.push_back(lineAlpha);

            // Third point (extrapolated)
            LinesRender2::Point p2;

            // Was 1000
            BL_FLOAT partialX1 = partial.mFreq + partial.mBeta0*VIEW_BETA0_COEFF;
            
            partialX1 =
                mViewScale->ApplyScale(mViewXScale, partialX1,
                                       (BL_FLOAT)0.0, (BL_FLOAT)(mSampleRate*0.5));
                                              
            p2.mX = partialX1 - 0.5;
            p2.mY = partial.mAmp;
            p2.mZ = 1.0;
            p2.mId = (int)partial.mId;

            // Blue
            p2.mR = 0;
            p2.mG = 0;
            p2.mB = 255;
            p2.mA = 255;
            
            lineBeta.push_back(p2);
            segments.push_back(lineBeta);
        }

        //
        int numSlices = mSASViewerRender->GetNumSlices();
        
        mPartialsSegments.push_back(segments);
        
        while(mPartialsSegments.size() > numSlices)
            mPartialsSegments.pop_front();
        
        // Update Z
        int divisor = mSASViewerRender->GetNumSlices();
        if (divisor <= 0)
            divisor = 1;
        BL_FLOAT incrZ = 1.0/divisor;

        for (int j = 0; j < mPartialsSegments.size(); j++)
        {
            vector<vector<LinesRender2::Point> > &segments = mPartialsSegments[j];

            for (int i = 0; i < segments.size(); i++)
            {
                vector<LinesRender2::Point> &seg = segments[i];

                if (seg.empty())
                    continue;
                
                BL_FLOAT z = seg[0].mZ;
                z -= incrZ;
            
                for (int k = 0; k < seg.size(); k++)
                {   
                    LinesRender2::Point &p = seg[k];

                    p.mZ = z;
                }
            }
        }
        
        BL_FLOAT lineWidth = 2.0;
        
        vector<LinesRender2::Line> &partialLines = mTmpBuf16;
        SegmentsToLines(mPartialsSegments, &partialLines);

        mSASViewerRender->SetAdditionalLines(DETECTION, partialLines, lineWidth);
    }
}

void
SASViewerProcess4::DisplayZombiePoints()
{
#define ZOMBIE_POINT_OFFSET_X 0.0025
    
    if (mSASViewerRender != NULL)
    {
        mSASViewerRender->ShowAdditionalPoints(DETECTION, mShowDetectionPoints);
        
        // Add points corresponding to raw detected partials
        vector<Partial> partials = mCurrentNormPartials;

        // Create line
        vector<LinesRender2::Point> line;
        for (int i = 0; i < partials.size(); i++)
        {
            const Partial &partial = partials[i];

            if (partial.mState != Partial::State::ZOMBIE)
                continue;
            
            LinesRender2::Point p;
            
            BL_FLOAT partialX = partial.mFreq;

            partialX =
                mViewScale->ApplyScale(mViewXScale, partialX,
                                       (BL_FLOAT)0.0, (BL_FLOAT)(mSampleRate*0.5));
                                              
            p.mX = partialX - 0.5;
            p.mY = partial.mAmp;
            
            p.mZ = 1.0;
            
            p.mId = (int)partial.mId;

            // Make a small offset, so we can see both points
            p.mX += ZOMBIE_POINT_OFFSET_X;
            
            line.push_back(p);
        }
        
        //
        int numSlices = mSASViewerRender->GetNumSlices();

        // Keep track of the points we pop
        vector<LinesRender2::Point> prevPoints;
        // Initialize, just in case
        if (!mPartialsPointsZombie.empty())
            prevPoints = mPartialsPointsZombie[0];
        
        mPartialsPointsZombie.push_back(line);
        
        while(mPartialsPointsZombie.size() > numSlices)
        {
            prevPoints = mPartialsPointsZombie[0];
            mPartialsPointsZombie.pop_front();
        }
        
        // Magenta
        unsigned char color[4] = { 255, 0, 255, 255 };
        
        // Set color
        for (int j = 0; j < mPartialsPointsZombie.size(); j++)
        {
            vector<LinesRender2::Point> &line2 = mPartialsPointsZombie[j];

            for (int i = 0; i < line2.size(); i++)
            {
                LinesRender2::Point &p = line2[i];

                // Color
                p.mR = color[0];
                p.mG = color[1];
                p.mB = color[2];
                p.mA = color[3];
            }
        }

        // Update Z
        int divisor = mSASViewerRender->GetNumSlices();
        if (divisor <= 0)
            divisor = 1;
        BL_FLOAT incrZ = 1.0/divisor;
        for (int j = 0; j < mPartialsPointsZombie.size(); j++)
        {
            vector<LinesRender2::Point> &line2 = mPartialsPointsZombie[j];

            if (line2.empty())
                continue;
            
            BL_FLOAT z = line2[0].mZ;
            z -= incrZ;

            for (int i = 0; i < line2.size(); i++)
            {
                LinesRender2::Point &p = line2[i];
                p.mZ = z; 
            }
        }
        
        BL_FLOAT lineWidth = 4.0;

        vector<LinesRender2::Line> &partialLines = mTmpBuf6;
        //PointsToLines(mPartialsPointsZombie, &partialLines);
        
        // Add all the points at the same time
        PointsToLinesMix(mPartialsPoints, mPartialsPointsZombie, &partialLines);
        
        mSASViewerRender->SetAdditionalPoints(DETECTION, partialLines, lineWidth);
    }
}

void
SASViewerProcess4::DisplayTracking()
{
    if (mSASViewerRender != NULL)
    {
        mSASViewerRender->ShowAdditionalLines(TRACKING, mShowTrackingLines);
        
        // Add the magnitudes
        WDL_TypedBuf<BL_FLOAT> &data = mTmpBuf8;
        mViewScale->ApplyScaleFilterBank(mViewXScaleFB, &data, mCurrentMagns,
                                         mSampleRate, mCurrentMagns.GetSize());
                                         
        mSASViewerRender->AddData(TRACKING, data);
        
        mSASViewerRender->SetLineMode(TRACKING, LinesRender2::LINES_FREQ);
        
        // Add lines corresponding to the well tracked partials
        vector<Partial> partials = mCurrentNormPartials;

        // Create blue lines from trackers
        vector<LinesRender2::Point> line;
        for (int i = 0; i < partials.size(); i++)
        {
            const Partial &partial = partials[i];
            
            LinesRender2::Point p;
            
            BL_FLOAT partialX = partial.mFreq;

            partialX =
                mViewScale->ApplyScale(mViewXScale, partialX,
                                       (BL_FLOAT)0.0, (BL_FLOAT)(mSampleRate*0.5));
                                       
            p.mX = partialX - 0.5;
            p.mY = partial.mAmp;
            
            p.mZ = 0.0;
            
            p.mId = (int)partial.mId;
            
            line.push_back(p);
        }

        //
        int numSlices = mSASViewerRender->GetNumSlices();
        
        // Keep track of the points we pop
        vector<LinesRender2::Point> prevPoints;
        // Initialize, just in case
        if (!mFilteredPartialsPoints.empty())
            prevPoints = mFilteredPartialsPoints[0];
        
        mFilteredPartialsPoints.push_back(line);
        
        while(mFilteredPartialsPoints.size() > numSlices)
        {
            prevPoints = mFilteredPartialsPoints[0];
            
            mFilteredPartialsPoints.pop_front();
        }
        
        CreateLines(prevPoints);
        
        // Set color
        for (int j = 0; j < mPartialLines.size(); j++)
        {
            LinesRender2::Line &line2 = mPartialLines[j];
            
            if (!line2.mPoints.empty())
            {
                // Default
                //line.mColor[0] = color[0];
                //line.mColor[1] = color[1];
                //line.mColor[2] = color[2];
                
                // Debug
                IdToColor(line2.mPoints[0].mId, line2.mColor);
                
                line2.mColor[3] = 255; // alpha
            }
        }
            
        BL_FLOAT lineWidth = 1.5;
        if (!mDebugPartials)
            lineWidth = 4.0;
        
        mSASViewerRender->SetAdditionalLines(TRACKING, mPartialLines, lineWidth);
    }
}

void
SASViewerProcess4::DisplayHarmo()
{
#if DISPLAY_HARMO_SUBSTRACT 
    // More smooth
    WDL_TypedBuf<BL_FLOAT> noise;
    mSASFrame->GetNoiseEnvelope(&noise);
    
    WDL_TypedBuf<BL_FLOAT> harmo = mCurrentMagns;
    BLUtils::SubstractValues(&harmo, noise);
    
    BLUtils::ClipMin(&harmo, (BL_FLOAT)0.0);
#else  
    // Should be more correct
    WDL_TypedBuf<BL_FLOAT> harmo;
    mPartialTracker->GetHarmonicEnvelope(&harmo);
#endif
    
    if (mSASViewerRender != NULL)
    {
        WDL_TypedBuf<BL_FLOAT> &data = mTmpBuf9;
        mViewScale->ApplyScaleFilterBank(mViewXScaleFB, &data, harmo,
                                         mSampleRate, harmo.GetSize());
        
        mSASViewerRender->AddData(HARMO, data);
        mSASViewerRender->SetLineMode(HARMO, LinesRender2::LINES_FREQ);
        
        mSASViewerRender->ShowAdditionalPoints(HARMO, false);
        mSASViewerRender->ShowAdditionalLines(HARMO, false);
    }
}

void
SASViewerProcess4::DisplayNoise()
{
    WDL_TypedBuf<BL_FLOAT> noise;
    //mSASFrame->GetNoiseEnvelope(&noise);
    mPartialTracker->GetNoiseEnvelope(&noise);
    
    if (mSASViewerRender != NULL)
    {
        WDL_TypedBuf<BL_FLOAT> &data = mTmpBuf10;
        mViewScale->ApplyScaleFilterBank(mViewXScaleFB, &data, noise,
                                         mSampleRate, noise.GetSize());
        
        mSASViewerRender->AddData(NOISE, data);
        mSASViewerRender->SetLineMode(NOISE, LinesRender2::LINES_FREQ);

        mSASViewerRender->ShowAdditionalPoints(NOISE, false);
        mSASViewerRender->ShowAdditionalLines(NOISE, false);
    }
}

void
SASViewerProcess4::DisplayAmplitude()
{
#define AMP_Y_COEFF 10.0 //20.0 //1.0
    
    BL_FLOAT amp = mSASFrame->GetAmplitude();
    
    WDL_TypedBuf<BL_FLOAT> amps;
    amps.Resize(mBufferSize/2);
    BLUtils::FillAllValue(&amps, amp);
    
    BLUtils::MultValues(&amps, (BL_FLOAT)AMP_Y_COEFF);
    
    if (mSASViewerRender != NULL)
    {
        mSASViewerRender->AddData(AMPLITUDE, amps);
        
        // WARNING: Won't benefit from straight lines optim
        mSASViewerRender->SetLineMode(AMPLITUDE,
                                      //LinesRender2::LINES_FREQ);
                                      LinesRender2::LINES_TIME);

        mSASViewerRender->ShowAdditionalPoints(AMPLITUDE, false);
        mSASViewerRender->ShowAdditionalLines(AMPLITUDE, false);
    }
}

void
SASViewerProcess4::DisplayFrequency()
{    
#define FREQ_Y_COEFF 40.0 //10.0
    
    BL_FLOAT freq = mSASFrame->GetFrequency();
  
    freq /= mSampleRate*0.5;

    WDL_TypedBuf<BL_FLOAT> freqs;
    freqs.Resize(mBufferSize/2);
    BLUtils::FillAllValue(&freqs, freq);
    
    BLUtils::MultValues(&freqs, (BL_FLOAT)FREQ_Y_COEFF);
    
    if (mSASViewerRender != NULL)
    {
        mSASViewerRender->AddData(FREQUENCY, freqs);
        
        // WARNING: Won't benefit from straight lines optim
        mSASViewerRender->SetLineMode(FREQUENCY,
                                      //LinesRender2::LINES_FREQ);
                                      LinesRender2::LINES_TIME);

        mSASViewerRender->ShowAdditionalPoints(FREQUENCY, false);
        mSASViewerRender->ShowAdditionalLines(FREQUENCY, false);
    }
}

void
SASViewerProcess4::DisplayColor()
{
    WDL_TypedBuf<BL_FLOAT> color;
    mSASFrame->GetColor(&color);
    
    BL_FLOAT amplitude = mSASFrame->GetAmplitude();
    
    BLUtils::MultValues(&color, amplitude);
    
    if (mPartialTracker != NULL)
        mPartialTracker->PreProcessDataXY(&color);
    
    if (mSASViewerRender != NULL)
    {
        WDL_TypedBuf<BL_FLOAT> &data = mTmpBuf13;
        mViewScale->ApplyScaleFilterBank(mViewXScaleFB, &data, color,
                                         mSampleRate, color.GetSize());
        
        mSASViewerRender->AddData(COLOR, data);
        mSASViewerRender->SetLineMode(COLOR, LinesRender2::LINES_FREQ);

        mSASViewerRender->ShowAdditionalPoints(COLOR, false);
        mSASViewerRender->ShowAdditionalLines(COLOR, false);
    }
}

void
SASViewerProcess4::DisplayWarping()
{
#define WARPING_COEFF 1.0
    
    WDL_TypedBuf<BL_FLOAT> warping;
    mSASFrame->GetNormWarping(&warping);
    
    if (mPartialTracker != NULL)
        mPartialTracker->PreProcessDataX(&warping);
    
    BLUtils::AddValues(&warping, (BL_FLOAT)-1.0);
    
    BLUtils::MultValues(&warping, (BL_FLOAT)WARPING_COEFF);
    
    if (mSASViewerRender != NULL)
    {
        WDL_TypedBuf<BL_FLOAT> &data = mTmpBuf14;
        mViewScale->ApplyScaleFilterBank(mViewXScaleFB, &data, warping,
                                         mSampleRate, warping.GetSize());
        
        mSASViewerRender->AddData(WARPING, data);
        mSASViewerRender->SetLineMode(WARPING, LinesRender2::LINES_FREQ);

        mSASViewerRender->ShowAdditionalPoints(WARPING, false);
        mSASViewerRender->ShowAdditionalLines(WARPING, false);
    }
}

int
SASViewerProcess4::FindIndex(const vector<int> &ids, int idx)
{
    if (idx == -1)
        return -1;
    
    for (int i = 0; i < ids.size(); i++)
    {
        if (ids[i] == idx)
            return i;
    }
    
    return -1;
}

int
SASViewerProcess4::FindIndex(const vector<LinesRender2::Point> &points, int idx)
{
    if (idx == -1)
        return -1;
    
    for (int i = 0; i < points.size(); i++)
    {
        if (points[i].mId == idx)
            return i;
    }
    
    return -1;
}

// Optimized version
void
SASViewerProcess4::CreateLines(const vector<LinesRender2::Point> &prevPoints)
{
    if (mSASViewerRender == NULL)
        return;
    
    if (mFilteredPartialsPoints.empty())
        return;
    
    // Update z for the current line points
    //
    int divisor = mSASViewerRender->GetNumSlices(); // - 1;
    if (divisor <= 0)
        divisor = 1;
    BL_FLOAT incrZ = 1.0/divisor;
    for (int i = 0; i < mPartialLines.size(); i++)
    {
        LinesRender2::Line &line = mPartialLines[i];
        for (int j = 0; j < line.mPoints.size(); j++)
        {
            LinesRender2::Point &p = line.mPoints[j];
            
            p.mZ -= incrZ;
        }
    }
    
    // Shorten the lines if they are too long
    vector<LinesRender2::Line> newLines;
    for (int i = 0; i < mPartialLines.size(); i++)
    {
        const LinesRender2::Line &line = mPartialLines[i];
        
        LinesRender2::Line newLine;
        for (int j = 0; j < line.mPoints.size(); j++)
        {
            const LinesRender2::Point &p = line.mPoints[j];
            if (p.mZ > 0.0)
                newLine.mPoints.push_back(p);
        }
        
        if (!newLine.mPoints.empty())
            newLines.push_back(newLine);
    }
    
    // Update the current partial lines
    mPartialLines = newLines;
    newLines.clear();
    
    // Create the new lines
    const vector<LinesRender2::Point> &newPoints =
        mFilteredPartialsPoints[mFilteredPartialsPoints.size() - 1];
    for (int i = 0; i < newPoints.size(); i++)
    {
        LinesRender2::Point newPoint = newPoints[i];

        // Adjust
        newPoint.mZ = 1.0 - incrZ;
        
        bool pointAdded = false;
        
        // Search for previous lines to be continued
        for (int j = 0; j < mPartialLines.size(); j++)
        {
            LinesRender2::Line &prevLine = mPartialLines[j];
        
            if (!prevLine.mPoints.empty())
            {
                int lineIdx = prevLine.mPoints[0].mId;
                
                if (lineIdx == newPoint.mId)
                {
                    // Add the point to prev line
                    prevLine.mPoints.push_back(newPoint);
                    
                    // We are done
                    pointAdded = true;
                    
                    break;
                }
            }
        }
        
        // Create a new line ?
        if (!pointAdded)
        {
            LinesRender2::Line newLine;
            newLine.mPoints.push_back(newPoint);
            
            mPartialLines.push_back(newLine);
        }
    }
}

void
SASViewerProcess4::PointsToLines(const deque<vector<LinesRender2::Point> > &points,
                                 vector<LinesRender2::Line> *lines)
{
    lines->resize(points.size());
    for (int i = 0; i < lines->size(); i++)
    {
        LinesRender2::Line &line = (*lines)[i];
        line.mPoints = points[i];
        
        // Dummy color
        line.mColor[0] = 0;
        line.mColor[1] = 0;
        line.mColor[2] = 0;
        line.mColor[3] = 0;
    }
}

void
SASViewerProcess4::
PointsToLinesMix(const deque<vector<LinesRender2::Point> > &points0,
                 const deque<vector<LinesRender2::Point> > &points1,
                 vector<LinesRender2::Line> *lines)
{
    lines->resize(points0.size());
    for (int i = 0; i < lines->size(); i++)
    {
        LinesRender2::Line &line = (*lines)[i];
        line.mPoints = points0[i];

        for (int j = 0; j < points1[i].size(); j++)
        {
            const LinesRender2::Point &p = points1[i][j];
            line.mPoints.push_back(p);
        }
        
        // Dummy color
        line.mColor[0] = 0;
        line.mColor[1] = 0;
        line.mColor[2] = 0;
        line.mColor[3] = 0;
    }
}

void
SASViewerProcess4::
SegmentsToLines(const deque<vector<vector<LinesRender2::Point> > > &segments,
                vector<LinesRender2::Line> *lines)
{    
    if (segments.empty())
    {
        lines->clear();
        
        return;
    }
    
    // Count the total number of lines
    int numLines = 0;
    for (int i = 0; i < segments.size(); i++)
    {
        const vector<vector<LinesRender2::Point> > &seg0 = segments[i];
        
        for (int j = 0; j < seg0.size(); j++)
        {
            const vector<LinesRender2::Point> &seg = seg0[j];

            if (seg.size() != 2)
                continue;

            numLines++;
        }
    }

    lines->resize(numLines);

    // Process
    int lineNum = 0;
    LinesRender2::Line line;
    line.mPoints.resize(2);
    for (int i = 0; i < segments.size(); i++)
    {
        const vector<vector<LinesRender2::Point> > &seg0 = segments[i];
        
        for (int j = 0; j < seg0.size(); j++)
        {
            const vector<LinesRender2::Point> &seg = seg0[j];

            if (seg.size() != 2)
                continue;

            line.mPoints[0] = seg[0];
            line.mPoints[1] = seg[1];
            
            // Take the color of the last point
            line.mColor[0] = seg[1].mR;
            line.mColor[1] = seg[1].mG;
            line.mColor[2] = seg[1].mB;
            line.mColor[3] = seg[1].mA;

            (*lines)[lineNum++] = line;
        }
    }
}

#endif // IGRAPHICS_NANOVG
