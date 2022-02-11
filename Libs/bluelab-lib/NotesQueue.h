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
