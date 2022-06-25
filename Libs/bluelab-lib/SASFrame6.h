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
//  SASFrame6.h
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#ifndef __BL_SASViewer__SASFrame6__
#define __BL_SASViewer__SASFrame6__

#include <vector>
using namespace std;

#include <Partial.h>

#include "IPlug_include_in_plug_hdr.h"

// SASFrame2: from SASFrame
//
// Use PartialToFreq compute everything for
// frequency in a sperate object
//
// Use PartialToFreq2 (improved algorithm)
//
// SASFrame5: when restarted developing, in order to make SASSynth
// Try to improve frequency computation, using same thenique as in Chroma

// SASFrame6: keep only the data. Processing is now in SASFrameAna and SASFrameSynth
class SASFrame6
{
public:
    class SASPartial
    {
    public:
        SASPartial();
        
        SASPartial(const SASPartial &other);
        SASPartial(const Partial &other);
        
        virtual ~SASPartial();
        
        static bool AmpLess(const SASPartial &p1, const SASPartial &p2);

    public:
        // Values are normalized, as provided by SASViewerProcess
        BL_FLOAT mFreq;
        BL_FLOAT mAmp;
        BL_FLOAT mPhase;

        int mLinkedId;
        enum Partial::State mState;
        bool mWasAlive;
    };
    
    SASFrame6();
    SASFrame6(int bufferSize, BL_FLOAT sampleRate,
              int overlapping, int freqRes);
    
    virtual ~SASFrame6();
    
    void Reset(BL_FLOAT sampleRate);
    
    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate);
    
    // SAS params
    //
    void SetNoiseEnvelope(const WDL_TypedBuf<BL_FLOAT> &noiseEnv);
    void GetNoiseEnvelope(WDL_TypedBuf<BL_FLOAT> *noiseEnv) const;
    
    void SetAmplitude(BL_FLOAT amp);
    BL_FLOAT GetAmplitude() const;

    void SetFrequency(BL_FLOAT freq);
    BL_FLOAT GetFrequency() const;

    void SetColor(const WDL_TypedBuf<BL_FLOAT> &color);
    void GetColor(WDL_TypedBuf<BL_FLOAT> *color, bool applyFactor = true) const;

    void SetWarping(const WDL_TypedBuf<BL_FLOAT> &warping);
    void GetWarping(WDL_TypedBuf<BL_FLOAT> *warping) const;

    void SetWarpingInv(const WDL_TypedBuf<BL_FLOAT> &warpingInv);
    void GetWarpingInv(WDL_TypedBuf<BL_FLOAT> *warpingInv) const;
    
    // For "raw"
    void SetPartials(const vector<Partial> &partials);
    void SetPartials(const vector<SASPartial> &partials);
    void GetPartials(vector<SASPartial> *partials) const;

    // Onsets
    void SetOnsetDetected(bool flag);
    bool GetOnsetDetected() const;
        
    // Factors
    void SetAmpFactor(BL_FLOAT factor);
    void SetFreqFactor(BL_FLOAT factor);
    void SetColorFactor(BL_FLOAT factor);
    void SetWarpingFactor(BL_FLOAT factor);
    
    static void MixFrames(SASFrame6 *result,
                          const SASFrame6 &frame0,
                          const SASFrame6 &frame1,
                          BL_FLOAT t, bool mixFreq);
    
protected:    
    int mBufferSize;
    BL_FLOAT mSampleRate;
    int mOverlapping;
    int mFreqRes;
    
    WDL_TypedBuf<BL_FLOAT> mNoiseEnvelope;
    BL_FLOAT mAmplitude;
    BL_FLOAT mFrequency;
    WDL_TypedBuf<BL_FLOAT> mColor;
    WDL_TypedBuf<BL_FLOAT> mWarping;

    WDL_TypedBuf<BL_FLOAT> mWarpingInv;

    vector<SASPartial> mPartials;

    bool mOnsetDetected;
    
    BL_FLOAT mAmpFactor;
    BL_FLOAT mFreqFactor;
    BL_FLOAT mColorFactor;
    BL_FLOAT mWarpingFactor;
};

#endif /* defined(__BL_SASViewer__SASFrame6__) */
