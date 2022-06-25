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
 
#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "InfraSynthNotesQueue.h"

InfraSynthNotesQueue::InfraSynthNotesQueue(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mId = 0;

#if OPTIM1
    mPadAmp = DBToAmp(GAIN_PAD);
#endif
    
#if ADSR_SMOOTH_FEATURE
    mADSRSmoothFactor = 0.0;
#endif
    
#if ADSR_ADVANCED_PARAMS_FEATURE
    mAttackPeak = 1.0;
    mAttackShape = 1.0;
    
    mDecayPeak = 0.0;
    mDecayShape = 1.0;
    
    mSustainDelay = 3.0;
    mSustainShape = 1.0;
    
    mReleaseShape = 1.0;
    
    mEnd = 1.0;
    mEndPeak = 0.0;
    mEndShape = 1.0;
#endif
    
    mMainOscMix = 1.0;
    mNoiseMix = 0.0;
    mNoiseHPFFreq = 1.0;
    
    mRandGen = new RandomSequenceOfUnique(142351, 575672);
    mNoiseHPFFreq = 1.0;
    
    //
    mOscSync = false;
    
    mGenNoise = true;

    mInfraSynthProcess = NULL;
}

InfraSynthNotesQueue::~InfraSynthNotesQueue()
{
    for (int i = 0; i < mNotes.size(); i++)
    {
        Note *note = mNotes[i];
        delete note;
    }
    
    delete mRandGen;
}

void
InfraSynthNotesQueue::Reset()
{
    for (int i = 0; i < mNotes.size(); i++)
    {
        Note *note = mNotes[i];
        delete note;
    }
    
    mNotes.clear();
}

void
InfraSynthNotesQueue::SetSampleRate(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    for (int i = 0; i < mNotes.size(); i++)
    {
        Note *note = mNotes[i];
        
        note->mOscillator.setSampleRate(sampleRate);
        note->mEnvGen.SetSampleRate(sampleRate);
    }
}

void
InfraSynthNotesQueue::SetInfraSynthProcess(InfraSynthProcess *process)
{
    mInfraSynthProcess = process;
}

void
InfraSynthNotesQueue::AddNote(int keyNumber,
                    BL_FLOAT freq, BL_FLOAT velocity,
                    BL_FLOAT a, BL_FLOAT d, BL_FLOAT s, BL_FLOAT r,
                    bool muteMainOsc, bool useFixedPhantomFreq)
{
    // ??
    RemoveFinishedNotes();
    
    // Then add note
    Note *newNote = new Note();
    
    newNote->mKeyNumber = keyNumber;
    
    newNote->mOscillator.setMode(OSCILLATOR_MODE_SINE);
    newNote->mOscillator.setFrequency(freq);
    newNote->mOscillator.setSampleRate(mSampleRate);
    
#if !MUTE_MAIN_OSC_FEATURE
    newNote->mOscillator.setMuted(false);
#else
    newNote->mOscillator.setMuted(muteMainOsc);
#endif
    
#if OSC_SYNC_PHASES
    if (mOscSync)
        newNote->mOscillator.setPhase(0.5*M_PI);
#endif
    
    newNote->mEnvGen.SetSampleRate(mSampleRate);

#if 1 // FIXED
    newNote->mEnvGen.SetAttack(a);
    newNote->mEnvGen.SetDecay(d);
    newNote->mEnvGen.SetSustain(s);
    newNote->mEnvGen.SetRelease(r);
#endif
    
#if ADSR_ADVANCED_PARAMS_FEATURE
    //newNote->mEnvGen.SetAttackPeak(mAttackPeak);
    newNote->mEnvGen.SetAttackShape(mAttackShape);
    
    newNote->mEnvGen.SetDecayPeak(mDecayPeak);
    newNote->mEnvGen.SetDecayShape(mDecayShape);
    
    newNote->mEnvGen.SetSustainDelay(mSustainDelay);
    newNote->mEnvGen.SetSustainShape(mSustainShape);
    
    newNote->mEnvGen.SetReleaseShape(mReleaseShape);
    
    newNote->mEnvGen.SetEnd(mEnd);
    newNote->mEnvGen.SetEndPeak(mEndPeak);
    newNote->mEnvGen.SetEndShape(mEndShape);
    
    newNote->mEnvGen.SetAttackPeak(mAttackPeak); //
#endif
    
    newNote->mEnvGen.EnterStage(EnvelopeGenerator3::ENVELOPE_STAGE_ATTACK);
    
    //
    newNote->mVelocity = velocity;
    
#if ADSR_SMOOTH_FEATURE
    newNote->mPrevEnvelope = 0.0;
#endif
    
#if OSC_SYNC_FEATURE && OSC_SYNC_REAL_SYNC
    newNote->mPrevOscValue = 0.0;
    newNote->mSyncNeeded = false;
#endif
    
    // Partial
    if (mInfraSynthProcess != NULL)
    {
        SineSynthSimple::Partial p;
        p.mFreq = freq;
        p.mId = mId++;
        
#if !INFRA_SYNTH_OPTIM3
        p.mAmpDB = 0.0; //MIN_DB;
#else
        p.mAmp = 1.0;
#endif
        
        p.mPhase = 0.0;
        
        mInfraSynthProcess->GeneratePhantomPartials(p, &newNote->mPhantomPartials,
                                                    useFixedPhantomFreq);
                
        for (int i = 0; i < newNote->mPhantomPartials.size(); i++)
        {
#if !INFRA_SYNTH_OPTIM3
            BL_FLOAT ampDB = newNote->mPhantomPartials[i].mAmpDB;
            newNote->mPhantomInitAmpDB.push_back(ampDB);
#else
            BL_FLOAT amp = newNote->mPhantomPartials[i].mAmp;
            newNote->mPhantomInitAmp.push_back(amp);
#endif
        }
        
        mInfraSynthProcess->GenerateSubPartials(p, &newNote->mSubPartials);
        for (int i = 0; i < newNote->mSubPartials.size(); i++)
        {
#if !INFRA_SYNTH_OPTIM3
            BL_FLOAT ampDB = newNote->mSubPartials[i].mAmpDB;
            newNote->mSubInitAmpDB.push_back(ampDB);
#else
            BL_FLOAT amp = newNote->mSubPartials[i].mAmp;
            newNote->mSubInitAmp.push_back(amp);
#endif
        }
    }
    
    mNotes.push_back(newNote);
}

void
InfraSynthNotesQueue::ReleaseNote(int keyNumber)
{
    for (int i = 0; i < mNotes.size(); i++)
    {
        Note *note = mNotes[i];
        
        if (note->mKeyNumber == keyNumber)
        {
            if ((note->mEnvGen.GetCurrentStage() != EnvelopeGenerator3::ENVELOPE_STAGE_OFF) &&
                (note->mEnvGen.GetCurrentStage() != EnvelopeGenerator3::ENVELOPE_STAGE_RELEASE))
                note->mEnvGen.EnterStage(EnvelopeGenerator3::ENVELOPE_STAGE_RELEASE);
        }
    }
}

BL_FLOAT
InfraSynthNotesQueue::NextSample(BL_FLOAT *resPhantomSamp,
                                 BL_FLOAT *resSubSamp,
                                 BL_FLOAT *resNoiseSamp)
{
    BL_FLOAT samp = 0.0;
    
    if (resPhantomSamp != NULL)
        *resPhantomSamp = 0.0;
    if (resSubSamp != NULL)
        *resSubSamp = 0.0;
    
    //vector<SineSynthSimple::Partial> partials;
    
#if !OPTIM_APPLY_PARAM_SHAPE
    BL_FLOAT mainOscMix = BLUtils::ApplyParamShape(mMainOscMix, 0.5);
#else
    BL_FLOAT mainOscMix = mMainOscMix;
#endif
    
    BL_FLOAT sumNoise = 0.0;
    
    int numNotes = mNotes.size();
    for (int i = 0; i < numNotes; i++)
    {
        Note *note = mNotes[i];
        
        // Synth of the current note (main oscillator)
        //
        
        BL_FLOAT osc = note->mOscillator.nextSample();
        
#if OSC_SYNC_FEATURE && OSC_SYNC_REAL_SYNC
        // Check for sync
        note->mSyncNeeded = false;
        if (mOscSync)
        {
            if (((osc >= 0.0) && (note->mPrevOscValue < 0.0)) ||
                ((osc <= 0.0) && (note->mPrevOscValue > 0.0)))
                note->mSyncNeeded = true;
        }
        note->mPrevOscValue = osc;
#endif
        
#if MIX_MAIN_OSC
        osc *= mainOscMix;
#endif
        
        BL_FLOAT env = note->mEnvGen.NextSample();
        
#if ADSR_SMOOTH_FEATURE
        BL_FLOAT newEnv = (1.0 - mADSRSmoothFactor)*env + mADSRSmoothFactor*note->mPrevEnvelope;
        note->mPrevEnvelope = newEnv;
        
        env = newEnv;
#endif
        
        BL_FLOAT gain = env*note->mVelocity;
        
#if 1 // Gain pad
        
#if !OPTIM1
        BL_FLOAT padAmp = DBToAmp(GAIN_PAD);
#else
        BL_FLOAT padAmp = mPadAmp;
#endif
        
        gain *= padAmp;
#endif

#if !DEBUG_DUMP_ENVELOPES
        samp += osc*gain;
        
        // Compute noise
        //
        
#if NOISE_GEN_FEATURE
        if (mGenNoise)
        {
            BL_FLOAT noise = ComputeNoiseSamp();
            BL_FLOAT noise0 = noise*gain*mNoiseMix*NOISE_COEFF;
        
            // FIX
            sumNoise += noise0;
        }
#endif
        
#else
        samp += newEnv;
#endif
        
        // Update phantom partials amp
        //
#if !INFRA_SYNTH_OPTIM3
        BL_FLOAT gainDB = BLUtils::AmpToDB(gain, 1e-15, MIN_DB);
#endif
        
        int numPhantomPartials = note->mPhantomPartials.size();
        for (int j = 0; j < numPhantomPartials; j++)
        {
            SineSynthSimple::Partial &p = note->mPhantomPartials[j];
            
#if !INFRA_SYNTH_OPTIM3
            BL_FLOAT ampDB = gainDB;
            ampDB += note->mPhantomInitAmpDB[j];
            p.mAmpDB = ampDB;
#else
            BL_FLOAT phantGain = gain;
            phantGain *= note->mPhantomInitAmp[j];
            p.mAmp = phantGain;
#endif
        }
        
        // Update sub partial amp
        //
        int numSubPartials = note->mSubPartials.size();
        for (int j = 0; j < numSubPartials; j++)
        {
            SineSynthSimple::Partial &p = note->mSubPartials[j];

#if !INFRA_SYNTH_OPTIM3
            BL_FLOAT ampDB = gainDB;
            ampDB += note->mSubInitAmpDB[j];
            p.mAmpDB = ampDB;
#else
            BL_FLOAT subGain = gain;
            subGain *= note->mSubInitAmp[j];
            p.mAmp = subGain;
#endif
        }
    }
    
    // FIX
    if (resNoiseSamp != NULL)
        *resNoiseSamp = sumNoise;
    
#if DEBUG_DUMP_ENVELOPES
    return samp;
#endif
    
    //
    BL_FLOAT ampSamp = 0.0;
    if (mInfraSynthProcess != NULL)
    {
        ampSamp = mInfraSynthProcess->AmplifyOriginSample(samp);
    }
    samp += ampSamp;
    
    BL_FLOAT phantomSubSamp = 0.0;
    
    BL_FLOAT phantomSamp = 0.0;
    BL_FLOAT subSamp = 0.0;
    for (int i = 0; i < mNotes.size(); i++)
    {
        Note *note = mNotes[i];
        
        if (mInfraSynthProcess != NULL)
        {
#if OSC_SYNC_FEATURE && OSC_SYNC_REAL_SYNC
            if (note->mSyncNeeded)
            {
                mInfraSynthProcess->TriggerSync();
                note->mSyncNeeded = false;
            }
#endif
            
            BL_FLOAT ps;
            BL_FLOAT ss;
            
            BL_FLOAT s = mInfraSynthProcess->NextSample(&note->mPhantomPartials,
                                                        &note->mSubPartials,
                                                        &ps, &ss);
            
            phantomSubSamp += s;
            
            phantomSamp += ps;
            subSamp += ss;
        }
    }
    
    // Result
    if ((resPhantomSamp == NULL) && (resSubSamp == NULL))
    {
        samp += phantomSubSamp;
    }
    else
    {
        if (resPhantomSamp != NULL)
            *resPhantomSamp = phantomSamp;
        
        if (resSubSamp != NULL)
            *resSubSamp = subSamp;
    }
    
    return samp;
}

void
InfraSynthNotesQueue::Update()
{
    RemoveFinishedNotes();
}

#if ADSR_SMOOTH_FEATURE
void
InfraSynthNotesQueue::SetADSRSmoothFactor(BL_FLOAT smoothFactor)
{
    mADSRSmoothFactor = smoothFactor;
}
#endif

#if ADSR_ADVANCED_PARAMS_FEATURE
void
InfraSynthNotesQueue::SetAttackPeak(BL_FLOAT gain)
{
    mAttackPeak = gain;
}

void
InfraSynthNotesQueue::SetAttackShape(BL_FLOAT shape)
{
    mAttackShape = shape;
}

void
InfraSynthNotesQueue::SetDecayPeak(BL_FLOAT peak)
{
    mDecayPeak = peak;
}

void
InfraSynthNotesQueue::SetDecayShape(BL_FLOAT shape)
{
    mDecayShape = shape;
}

void
InfraSynthNotesQueue::SetSustainDelay(BL_FLOAT delay)
{
    mSustainDelay = delay;
}

void
InfraSynthNotesQueue::SetSustainShape(BL_FLOAT shape)
{
    mSustainShape = shape;
}

void
InfraSynthNotesQueue::SetReleaseShape(BL_FLOAT shape)
{
    mReleaseShape = shape;
}

void
InfraSynthNotesQueue::SetEnd(BL_FLOAT end)
{
    mEnd = end;
}

void
InfraSynthNotesQueue::SetEndPeak(BL_FLOAT endPeak)
{
    mEndPeak = endPeak;
}

void
InfraSynthNotesQueue::SetEndShape(BL_FLOAT endShape)
{
    mEndShape = endShape;
}
#endif

void
InfraSynthNotesQueue::SetMainOscMix(BL_FLOAT mix)
{
    mMainOscMix = mix;
}

void
InfraSynthNotesQueue::SetNoiseMix(BL_FLOAT mix)
{
    mNoiseMix = mix;
}

void
InfraSynthNotesQueue::SetOscSync(bool flag)
{
    mOscSync = flag;
    
#if OSC_SYNC_PHASES
    if (mInfraSynthProcess != NULL)
        mInfraSynthProcess->EnableStartSync(flag);
#endif
}

void
InfraSynthNotesQueue::SetGenNoise(bool flag)
{
    mGenNoise = flag;
}

void
InfraSynthNotesQueue::RemoveFinishedNotes()
{
    // First, remove finished notes
    vector<Note *> newNotes;
    
    for (int i = 0; i < mNotes.size(); i++)
    {
        Note *note = mNotes[i];
        if (note->mEnvGen.GetCurrentStage() != EnvelopeGenerator3::ENVELOPE_STAGE_OFF)
            newNotes.push_back(note);
    }
    
    mNotes = newNotes;
}

BL_FLOAT
InfraSynthNotesQueue::ComputeNoiseNorm()
{
#define MAX_NOISE_VALUE UINT_MAX
#define MAX_NOISE_VALUE_INV 1.0/MAX_NOISE_VALUE
    
    BL_FLOAT n = (mRandGen->next() % MAX_NOISE_VALUE)*MAX_NOISE_VALUE_INV;
    
    return n;
}


BL_FLOAT
InfraSynthNotesQueue::ComputeNoiseSamp()
{
    BL_FLOAT n = ComputeNoiseNorm();
    
    // Center
    n = n*2.0 - 1.0;
    
    return n;
}
