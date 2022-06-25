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
 
#ifndef INFRA_SYNTH_NOTES_QUEUE_H
#define INFRA_SYNTH_NOTES_QUEUE_H

#include <vector>
using namespace std;

#include <randomsequence.h>

#include <BLTypes.h>

#include "Oscillator.h"
#include <EnvelopeGenerator3.h>

#include <SineSynthSimple.h>

#include <InfraSynthProcess.h>

#include <InfraSynth_defs.h>

#include "IPlug_include_in_plug_hdr.h"

class InfraSynthNotesQueue
{
 public:
    class Note
    {
    public:        
        //
        int mKeyNumber;
        
        Oscillator mOscillator;
        EnvelopeGenerator3 mEnvGen;
        
        vector<SineSynthSimple::Partial> mPhantomPartials;
        vector<SineSynthSimple::Partial> mSubPartials;
        
#if !INFRA_SYNTH_OPTIM3
        vector<BL_FLOAT> mPhantomInitAmpDB;
        vector<BL_FLOAT> mSubInitAmpDB;
#else
        vector<BL_FLOAT> mPhantomInitAmp;
        vector<BL_FLOAT> mSubInitAmp;
#endif
        
        BL_FLOAT mVelocity;
        
#if ADSR_SMOOTH_FEATURE
        BL_FLOAT mPrevEnvelope;
#endif
        
#if OSC_SYNC_FEATURE && OSC_SYNC_REAL_SYNC
        BL_FLOAT mPrevOscValue;
        bool mSyncNeeded;
#endif
    };
    
    InfraSynthNotesQueue(BL_FLOAT sampleRate);
    
    virtual ~InfraSynthNotesQueue();
    
    void Reset();
    
    void SetSampleRate(BL_FLOAT sampleRate);
    
    void SetInfraSynthProcess(InfraSynthProcess *process);
    
    void AddNote(int keyNumber,
                 BL_FLOAT freq, BL_FLOAT velocity,
                 BL_FLOAT a, BL_FLOAT d, BL_FLOAT s, BL_FLOAT r,
                 bool muteMainOsc, bool useFixedPhantomFreq);
    
    void ReleaseNote(int keyNumber);
    
    BL_FLOAT NextSample(BL_FLOAT *resPhantomSamp = NULL,
                        BL_FLOAT *resSubSamp = NULL,
                        BL_FLOAT *noiseSamp = NULL);
    
    void Update();
    
#if ADSR_SMOOTH_FEATURE
    void SetADSRSmoothFactor(BL_FLOAT smoothFactor);
#endif
    
#if ADSR_ADVANCED_PARAMS_FEATURE
    void SetAttackPeak(BL_FLOAT gain);
    void SetAttackShape(BL_FLOAT shape);

    void SetDecayPeak(BL_FLOAT peak);
    void SetDecayShape(BL_FLOAT shape);
    
    void SetSustainDelay(BL_FLOAT delay);
    void SetSustainShape(BL_FLOAT shape);
    
    void SetReleaseShape(BL_FLOAT shape);
    
    void SetEnd(BL_FLOAT end);
    void SetEndPeak(BL_FLOAT endPeak);
    void SetEndShape(BL_FLOAT endShape);
#endif
    
    void SetMainOscMix(BL_FLOAT mix);
    void SetNoiseMix(BL_FLOAT mix);
    
    void SetOscSync(bool flag);
    
    void SetGenNoise(bool flag);
    
    ///
    BL_FLOAT ComputeNoiseNorm();
    BL_FLOAT ComputeNoiseSamp();
    
 protected:
    void RemoveFinishedNotes();
    
    //
    BL_FLOAT mSampleRate;
    
    vector<Note *> mNotes;
    
    InfraSynthProcess *mInfraSynthProcess;
    
    unsigned long long mId;
    
#if OPTIM1
    BL_FLOAT mPadAmp;
#endif
    
#if ADSR_SMOOTH_FEATURE
    BL_FLOAT mADSRSmoothFactor;
#endif
    
#if ADSR_ADVANCED_PARAMS_FEATURE
    BL_FLOAT mAttackPeak;
    BL_FLOAT mAttackShape;
    
    BL_FLOAT mDecayPeak;
    BL_FLOAT mDecayShape;
    
    BL_FLOAT mSustainDelay;
    BL_FLOAT mSustainShape;
    
    BL_FLOAT mReleaseShape;

    BL_FLOAT mEnd;
    BL_FLOAT mEndPeak;
    BL_FLOAT mEndShape;
#endif
    
    BL_FLOAT mMainOscMix;
    BL_FLOAT mNoiseMix;
    
    BL_FLOAT mNoiseHPFFreq;
    RandomSequenceOfUnique *mRandGen;
    
    // Sync
    bool mOscSync;
    
    // Noise
    bool mGenNoise;
};

#endif
