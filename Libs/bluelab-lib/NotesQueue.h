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
 
#ifndef NOTES_QUEUE_H
#define NOTES_QUEUE_H

#include <vector>
using namespace std;

#include <EnvelopeGenerator3.h>

#define ADSR_SMOOTH_FEATURE 1

#define ADSR_ADVANCED_PARAMS_FEATURE 1

#define NQ_OPTIM1 1

class NotesQueue
{
public:
    struct Note
    {
        int mKeyNumber;
        
        EnvelopeGenerator3 mEnvGen;
        
        double mVelocity;
        
#if ADSR_SMOOTH_FEATURE
        double mPrevEnvelope;
#endif
    };
    
    NotesQueue(double sampleRate);
    
    virtual ~NotesQueue();
    
    void Reset();
    
    void SetSampleRate(double sampleRate);
    
    void AddNote(int keyNumber,
                 double freq, double velocity,
                 double a, double d, double s, double r);
    
    void ReleaseNote(int keyNumber);
    
    double NextSample();
    
    void Update();
    
#if ADSR_SMOOTH_FEATURE
    void SetADSRSmoothFactor(double smoothFactor);
#endif
    
#if ADSR_ADVANCED_PARAMS_FEATURE
    void SetAttackPeak(double gain);
    void SetAttackShape(double shape);

    void SetDecayPeak(double peak);
    void SetDecayShape(double shape);
    
    void SetSustainDelay(double delay);
    void SetSustainShape(double shape);
    
    void SetReleaseShape(double shape);
    
    void SetEnd(double end);
    void SetEndPeak(double endPeak);
    void SetEndShape(double endShape);
#endif
    
protected:
    void RemoveFinishedNotes();
    
    double mSampleRate;
    
    vector<Note *> mNotes;
    
    unsigned long long mId;
    
#if NQ_OPTIM1
    double mPadAmp;
#endif
    
#if ADSR_SMOOTH_FEATURE
    double mADSRSmoothFactor;
#endif
    
#if ADSR_ADVANCED_PARAMS_FEATURE
    double mAttackPeak;
    double mAttackShape;
    
    double mDecayPeak;
    double mDecayShape;
    
    double mSustainDelay;
    double mSustainShape;
    
    double mReleaseShape;

    double mEnd;
    double mEndPeak;
    double mEndShape;
#endif
};

#endif
