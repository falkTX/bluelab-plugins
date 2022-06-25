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
//  SASFrame3.h
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#ifndef __BL_SASViewer__SASFrame3__
#define __BL_SASViewer__SASFrame3__

#include <vector>
using namespace std;

#include <PartialTracker5.h>

// SASFrame2: from SASFrame
//
// Use PartialToFreq compute everything for
// frequency in a sperate object
//
// Use PartialToFreq2 (improved algorithm)
class PartialsToFreq5;
class FreqAdjustObj3;
class WavetableSynth;
class SASFrame3
{
public:
    enum SynthMode
    {
        FFT,
        OSC
    };
    
    class SASPartial
    {
    public:
        SASPartial();
        
        SASPartial(const SASPartial &other);
        
        virtual ~SASPartial();
        
        static bool AmpLess(const SASPartial &p1, const SASPartial &p2);

    public:
        // Values are normalized, as provided by SASViewerProcess
        BL_FLOAT mFreq;
        BL_FLOAT mAmp; // Still used ?
        BL_FLOAT mPhase;
    };
    
    SASFrame3(int bufferSize, BL_FLOAT sampleRate, int overlapping);
    
    virtual ~SASFrame3();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetMinAmpDB(BL_FLOAT ampDB);
    
    void SetSynthMode(enum SynthMode mode);
    
    // De-normalized partials
    void SetPartials(const vector<PartialTracker5::Partial> &partials);
    
    void SetNoiseEnvelope(const WDL_TypedBuf<BL_FLOAT> &noiseEnv);
    void GetNoiseEnvelope(WDL_TypedBuf<BL_FLOAT> *noiseEnv) const;
    
    // Get
    BL_FLOAT GetAmplitude() const;
    BL_FLOAT GetFrequency() const;
    void GetColor(WDL_TypedBuf<BL_FLOAT> *color) const;
    void GetNormWarping(WDL_TypedBuf<BL_FLOAT> *warping) const;
    
    // Set
    void SetAmplitude(BL_FLOAT amp);
    void SetFrequency(BL_FLOAT freq);
    void SetColor(const WDL_TypedBuf<BL_FLOAT> &color);
    void SetNormWarping(const WDL_TypedBuf<BL_FLOAT> &warping);
    
    // Compute directly from input partials
    void ComputeSamples(WDL_TypedBuf<BL_FLOAT> *samples);
    void ComputeSamplesPost(WDL_TypedBuf<BL_FLOAT> *samples);
    
    // Compute by resynthesizing from color, warping etc.
    void ComputeSamplesResynth(WDL_TypedBuf<BL_FLOAT> *samples);
    void ComputeSamplesResynthPost(WDL_TypedBuf<BL_FLOAT> *samples);
    
    void ComputeFftPartials(WDL_TypedBuf<BL_FLOAT> *samples);
    
    void SetPitch(BL_FLOAT pitch);
    
    void SetHarmonicSoundFlag(bool flag);
    
    bool ComputeSamplesFlag();
    bool ComputeSamplesPostFlag();
    
    static void MixFrames(SASFrame3 *result,
                          const SASFrame3 &frame0,
                          const SASFrame3 &frame1,
                          BL_FLOAT t, bool mixFreq);
    
protected:
    void ComputeSamplesPartials(WDL_TypedBuf<BL_FLOAT> *samples);
    
    void ComputeSamplesSAS(WDL_TypedBuf<BL_FLOAT> *samples);
    // Optim
    void ComputeSamplesSAS2(WDL_TypedBuf<BL_FLOAT> *samples);
    // Avoid clicks
    void ComputeSamplesSAS3(WDL_TypedBuf<BL_FLOAT> *samples);
    // Optim
    void ComputeSamplesSAS4(WDL_TypedBuf<BL_FLOAT> *samples);
    // Optim
    void ComputeSamplesSAS5(WDL_TypedBuf<BL_FLOAT> *samples);
    // Refact
    void ComputeSamplesSAS6(WDL_TypedBuf<BL_FLOAT> *samples);
    BL_FLOAT GetColor(const WDL_TypedBuf<BL_FLOAT> &color, BL_FLOAT binIdx);
    BL_FLOAT GetWarping(const WDL_TypedBuf<BL_FLOAT> &warping, BL_FLOAT binIdx);
    
    void ComputeFftSAS(WDL_TypedBuf<BL_FLOAT> *samples);
    void ComputeFftSASFreqAdjust(WDL_TypedBuf<BL_FLOAT> *samples);
    void ComputeSamplesSASTable(WDL_TypedBuf<BL_FLOAT> *samples);
    void ComputeSamplesSASTable2(WDL_TypedBuf<BL_FLOAT> *samples); // Better interpolation
    void ComputeSamplesSASOverlap(WDL_TypedBuf<BL_FLOAT> *samples);
    
    void Compute();
    
    // Compute steps
    //
    void ComputeAmplitude();
    
    void ComputeFrequency();
    
    void ComputeColor();
    void ComputeColorAux();
    
    void ComputeNormWarping();
    void ComputeNormWarpingAux();
    
    // Simple version
    BL_FLOAT ApplyNormWarping(BL_FLOAT freq);
    BL_FLOAT ApplyColor(BL_FLOAT freq);

    // Versions to interpolate over time
    BL_FLOAT ApplyNormWarping(BL_FLOAT freq, BL_FLOAT t);
    BL_FLOAT ApplyColor(BL_FLOAT freq, BL_FLOAT t);
    
    bool FindPartial(BL_FLOAT freq);

    void GetPartial(PartialTracker5::Partial *result, int index, BL_FLOAT t);
    
    int FindPrevPartialIdx(int currentPartialIdx);

    void GetSASPartial(SASPartial *result, int index, BL_FLOAT t);
    
    // Estimate the fundamental frequency
    BL_FLOAT TWMEstimate(const vector<PartialTracker5::Partial> &partials);
    BL_FLOAT ComputeTWMError(const vector<PartialTracker5::Partial> &partials,
                           BL_FLOAT testFreq);
    BL_FLOAT ComputeErrorK(const PartialTracker5::Partial &partial,
                         BL_FLOAT harmo, BL_FLOAT Amax);
    BL_FLOAT ComputeErrorN(const PartialTracker5::Partial &partial,
                         BL_FLOAT harmo, BL_FLOAT Amax);
    BL_FLOAT GetNearestHarmonic(BL_FLOAT freq, BL_FLOAT refFreq);
    
    
    BL_FLOAT GetFreq(BL_FLOAT freq0, BL_FLOAT freq1, BL_FLOAT t);
    BL_FLOAT GetAmp(BL_FLOAT amp0, BL_FLOAT amp1, BL_FLOAT t);
    BL_FLOAT GetCol(BL_FLOAT col0, BL_FLOAT col1, BL_FLOAT t);
    
    //
    SynthMode mSynthMode;
    
    // Tracked partials
    BL_FLOAT mPrevAmplitude;
    
    // Not normalized
    vector<PartialTracker5::Partial> mPartials;
    vector<PartialTracker5::Partial> mPrevPartials;
    
    BL_FLOAT mAmplitude;
    
    BL_FLOAT mFrequency;
    // For smoothing
    BL_FLOAT mPrevFrequency;
    
    int mBufferSize;
    BL_FLOAT mSampleRate;
    int mOverlapping;
    
    WDL_TypedBuf<BL_FLOAT> mNoiseEnvelope;
    
    WDL_TypedBuf<BL_FLOAT> mColor;
    WDL_TypedBuf<BL_FLOAT> mNormWarping;
    
    // Must keep the prev values, to interpolate over time
    // when generating the samples
    WDL_TypedBuf<BL_FLOAT> mPrevColor;
    WDL_TypedBuf<BL_FLOAT> mPrevNormWarping;
    
    vector<SASPartial> mSASPartials;
    vector<SASPartial> mPrevSASPartials;
    
    BL_FLOAT mPitch;
    
    PartialsToFreq5 *mPartialsToFreq;
    
    FreqAdjustObj3 *mFreqObj;
    
    // For sample synth with table
    WavetableSynth *mTableSynth;
    
    BL_FLOAT mMinAmpDB;

    Scale *mScale;
};

#endif /* defined(__BL_SASViewer__SASFrame3__) */
