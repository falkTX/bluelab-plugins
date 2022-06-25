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

#include "NotesQueue.h"

#define GAIN_PAD 0.0 //-10.0

NotesQueue::NotesQueue(double sampleRate)
{
    mSampleRate = sampleRate;
    
    mId = 0;

#if NQ_OPTIM1
    mPadAmp = BLUtils::DBToAmp(GAIN_PAD);
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
}

NotesQueue::~NotesQueue()
{
    for (int i = 0; i < mNotes.size(); i++)
    {
        Note *note = mNotes[i];
        delete note;
    }
}

void
NotesQueue::Reset()
{
    for (int i = 0; i < mNotes.size(); i++)
    {
        Note *note = mNotes[i];
        delete note;
    }
    
    mNotes.clear();
}

void
NotesQueue::SetSampleRate(double sampleRate)
{
    mSampleRate = sampleRate;
    
    for (int i = 0; i < mNotes.size(); i++)
    {
        Note *note = mNotes[i];
        note->mEnvGen.SetSampleRate(sampleRate);
    }
}

void
NotesQueue::AddNote(int keyNumber,
                    double freq, double velocity,
                    double a, double d, double s, double r)
{
    // ??
    RemoveFinishedNotes();
    
    // Then add note
    Note *newNote = new Note();
    
    newNote->mKeyNumber = keyNumber;
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
    
    newNote->mVelocity = velocity;
    
#if ADSR_SMOOTH_FEATURE
    newNote->mPrevEnvelope = 0.0;
#endif
    
    mNotes.push_back(newNote);
}

void
NotesQueue::ReleaseNote(int keyNumber)
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

double
NotesQueue::NextSample()
{
    double samp = 0.0;
    
    int numNotes = mNotes.size();
    for (int i = 0; i < numNotes; i++)
    {
        Note *note = mNotes[i];
        
        // Synth of the current ENVELOPE VALUE        
        double env = note->mEnvGen.NextSample();
        
#if ADSR_SMOOTH_FEATURE
        double newEnv = (1.0 - mADSRSmoothFactor)*env + mADSRSmoothFactor*note->mPrevEnvelope;
        note->mPrevEnvelope = newEnv;
        
        env = newEnv;
#endif
        
        double gain = env*note->mVelocity;
        
#if 1 // Gain pad
        
#if !NQ_OPTIM1
        double padAmp = DBToAmp(GAIN_PAD);
#else
        double padAmp = mPadAmp;
#endif
        
        gain *= padAmp;
#endif

#if !DEBUG_DUMP_ENVELOPES
        samp += gain;
#else
        samp += newEnv;
#endif
    }
    
    return samp;
}

void
NotesQueue::Update()
{
    RemoveFinishedNotes();
}

#if ADSR_SMOOTH_FEATURE
void
NotesQueue::SetADSRSmoothFactor(double smoothFactor)
{
    mADSRSmoothFactor = smoothFactor;
}
#endif

#if ADSR_ADVANCED_PARAMS_FEATURE
void
NotesQueue::SetAttackPeak(double gain)
{
    mAttackPeak = gain;
}

void
NotesQueue::SetAttackShape(double shape)
{
    mAttackShape = shape;
}

void
NotesQueue::SetDecayPeak(double peak)
{
    mDecayPeak = peak;
}

void
NotesQueue::SetDecayShape(double shape)
{
    mDecayShape = shape;
}

void
NotesQueue::SetSustainDelay(double delay)
{
    mSustainDelay = delay;
}

void
NotesQueue::SetSustainShape(double shape)
{
    mSustainShape = shape;
}

void
NotesQueue::SetReleaseShape(double shape)
{
    mReleaseShape = shape;
}

void
NotesQueue::SetEnd(double end)
{
    mEnd = end;
}

void
NotesQueue::SetEndPeak(double endPeak)
{
    mEndPeak = endPeak;
}

void
NotesQueue::SetEndShape(double endShape)
{
    mEndShape = endShape;
}
#endif

void
NotesQueue::RemoveFinishedNotes()
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
