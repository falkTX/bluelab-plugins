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
//  JReverb.h
//  UST
//
//  Created by applematuer on 8/29/20.
//
//

#ifndef __UST__JReverb__
#define __UST__JReverb__

#include <BLReverb.h>

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"


// From juce_Reverb

/*
 Performs a simple reverb effect on a stream of audio data.
     
  This is a simple stereo reverb, based on the technique and tunings used in FreeVerb.
  Use setSampleRate() to prepare it, and then call processStereo() or processMono() to
  apply the reverb to your audio data.
*/
class JReverbCombFilter;
class JReverbAllPassFilter;
class JReverb : public BLReverb
{
public:
    /** Holds the parameters being used by a Reverb object. */
    struct JReverbParams
    {
        BL_FLOAT mRoomSize   = 0.5;     /**< Room size, 0 to 1.0, where 1.0 is big, 0 is small. */
        BL_FLOAT mDamping    = 0.5;     /**< Damping, 0 to 1.0, where 0 is not damped, 1.0 is fully damped. */
        BL_FLOAT mWetLevel   = 0.33;    /**< Wet level, 0 to 1.0 */
        BL_FLOAT mDryLevel   = 0.4;     /**< Dry level, 0 to 1.0 */
        BL_FLOAT mWidth      = 1.0;     /**< Reverb width, 0 to 1.0, where 1.0 is very wide. */
        BL_FLOAT mFreezeMode = 0.0;     /**< Freeze mode - values < 0.5 are "normal" mode, values > 0.5
                                         put the reverb into a continuous feedback loop. */
    };
    
    JReverb();
    
    JReverb(const JReverb &other);
    
    virtual ~JReverb();
    
    virtual BLReverb *Clone() const override;
    
    void Reset(BL_FLOAT sampleRate, int blockSize) override;
    
    /// Returns the reverb's current parameters.
    const JReverbParams& GetParams() const;
        
    /*
     Applies a new set of parameters to the reverb.
     Note that this doesn't attempt to lock the reverb, so if you call this in parallel with
     the process method, you may get artifacts.
     */
    void SetParams(const JReverbParams &newParams);
        
    /*
     Sets the sample rate that will be used for the reverb.
     You must call this before the process methods, in order to tell it the correct sample rate.
    */
    void SetSampleRate(BL_FLOAT sampleRate);
    
    // Clears the reverb's buffers.
    void Reset();
    
    // Applies the reverb to two stereo channels of audio data.
    void ProcessStereo(BL_FLOAT *left, BL_FLOAT *right, int numSamples);
    
    // Applies the reverb to a single mono channel of audio data.
    void ProcessMono(BL_FLOAT *samples, int numSamples);
    
    // Mono
    void Process(const WDL_TypedBuf<BL_FLOAT> &input,
                 WDL_TypedBuf<BL_FLOAT> *outputL,
                 WDL_TypedBuf<BL_FLOAT> *outputR) override;
    
    // Stereo
    void Process(const WDL_TypedBuf<BL_FLOAT> inputs[2],
                 WDL_TypedBuf<BL_FLOAT> *outputL,
                 WDL_TypedBuf<BL_FLOAT> *outputR) override;
    
private:
    
    static bool IsFrozen(BL_FLOAT freezeMode);
        
    void UpdateDamping();
    
    void SetDamping (BL_FLOAT dampingToUse, BL_FLOAT roomSizeToUse);
    
    // ORIGIN
    enum { mNumCombs = 8, mNumAllPasses = 4, mNumChannels = 2 };
    
    // TEST: perfs are quite similar, sound looks better !!
    //enum { mNumCombs = 4, mNumAllPasses = 2, mNumChannels = 2 };
    
    JReverbParams mParams;
    BL_FLOAT mGain;
        
    JReverbCombFilter *mComb[mNumChannels][mNumCombs];
    JReverbAllPassFilter *mAllPass[mNumChannels][mNumAllPasses];
        
    BL_FLOAT mDamping;
    BL_FLOAT mFeedback;
    BL_FLOAT mDryGain;
    BL_FLOAT mWetGain1;
    BL_FLOAT mWetGain2;
};

#endif /* defined(__UST__JReverb__) */
