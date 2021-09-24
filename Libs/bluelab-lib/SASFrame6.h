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
    void GetColor(WDL_TypedBuf<BL_FLOAT> *color) const;

    void SetWarping(const WDL_TypedBuf<BL_FLOAT> &warping);
    void GetWarping(WDL_TypedBuf<BL_FLOAT> *warping) const;

    void SetWarpingInv(const WDL_TypedBuf<BL_FLOAT> &warpingInv);
    void GetWarpingInv(WDL_TypedBuf<BL_FLOAT> *warpingInv) const;
    
    // For "raw"
    void SetPartials(const vector<Partial> &partials);
    void GetPartials(vector<Partial> *partials) const;
    
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

    vector<Partial> mPartials;
    
    BL_FLOAT mAmpFactor;
    BL_FLOAT mFreqFactor;
    BL_FLOAT mColorFactor;
    BL_FLOAT mWarpingFactor;
};

#endif /* defined(__BL_SASViewer__SASFrame6__) */
