#include <stdio.h>

#include <cmath>
#include <algorithm>
using namespace std;

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include "PartialFilterAMFM.h"

#define MAX_ZOMBIE_AGE 5 //2
#define PARTIALS_HISTORY_SIZE 2 //2


#define EXTRAPOLATE_KALMAN 0 //1 //0
// Propagate dead and zombies with alpha0 and beta0
// Problem: at partial crossing, alpha0 (for amp) sometimes has big values 
#define EXTRAPOLATE_AMFM 0 //1

PartialFilterAMFM::PartialFilterAMFM(int bufferSize, BL_FLOAT sampleRate)
{
    //sampleRate = 2.0; // TEST
    
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
}

PartialFilterAMFM::~PartialFilterAMFM() {}

void
PartialFilterAMFM::Reset(int bufferSize, BL_FLOAT sampleRate)
{
    //sampleRate = 2.0; // TEST
    
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    mPartials.clear();
}
           
void
PartialFilterAMFM::FilterPartials(vector<Partial> *partials)
{
    for (int i = 0; i < partials->size(); i++)
    {
        Partial &p = (*partials)[i];
        p.mFreq *= mSampleRate*0.5;
        p.mBeta0 *= mSampleRate*0.5;
    }
    
    mPartials.push_front(*partials);
    
    while(mPartials.size() > PARTIALS_HISTORY_SIZE)
        mPartials.pop_back();
    
    partials->clear();
    
    if (mPartials.empty())
        return;
    
    if (mPartials.size() == 1)
        // Assigne ids to the first series of partials
    {
        for (int j = 0; j < mPartials[0].size(); j++)
        {
            Partial &currentPartial = mPartials[0][j];
            currentPartial.GenNewId();
        }
        
        // Not enough partials to filter, need 2 series
        return;
    }
    
    if (mPartials.size() < 2)
        return;
    
    const vector<Partial> &prevPartials = mPartials[1];
    vector<Partial> &currentPartials = mTmpPartials0;
    currentPartials = mPartials[0];
    
    // Partials that was not associated at the end
    vector<Partial> &remainingPartials = mTmpPartials1;
    remainingPartials.resize(0);
    
    AssociatePartialsAMFM(prevPartials, &currentPartials, &remainingPartials);
    
    // Add the new zombie and dead partials
    for (int i = 0; i < prevPartials.size(); i++)
    {
        const Partial &prevPartial = prevPartials[i];

        bool found = false;
        for (int j = 0; j < currentPartials.size(); j++)
        {
            const Partial &currentPartial = currentPartials[j];
            
            if (currentPartial.mId == prevPartial.mId)
            {
                found = true;
                
                break;
            }
        }

        if (!found)
        {
            if (prevPartial.mState == Partial::ALIVE)
            {
                // We set zombie for 1 frame only
                Partial newPartial = prevPartial;
                newPartial.mState = Partial::ZOMBIE;
                newPartial.mZombieAge = 0;

#if 1 // Good, extrapolate zombies
                
#if EXTRAPOLATE_KALMAN
                //newPartial.mPredictedFreq =
                //newPartial.mFreq = newPartial.mKf.updateEstimate(newPartial.mFreq);
                ExtrapolatePartialKalman(&newPartial);
#endif

#if EXTRAPOLATE_AMFM
                ExtrapolatePartialAMFM(&newPartial);
#endif

#endif
                // If MAX_ZOMBIE_AGE is 0, do not generate zombies
                if (newPartial.mZombieAge < MAX_ZOMBIE_AGE)
                    currentPartials.push_back(newPartial);
            }
            else if (prevPartial.mState == Partial::ZOMBIE)
            {
                Partial newPartial = prevPartial;
                
                newPartial.mZombieAge++;
                if (newPartial.mZombieAge >= MAX_ZOMBIE_AGE)
                {
                    newPartial.mState = Partial::DEAD;
                }
                else
                {
#if 1
                
#if EXTRAPOLATE_KALMAN
                    //newPartial.mPredictedFreq =
                    //newPartial.mFreq = newPartial.mKf.updateEstimate(newPartial.mFreq);
                    ExtrapolatePartialKalman(&newPartial);
#endif

#if EXTRAPOLATE_AMFM
                    ExtrapolatePartialAMFM(&newPartial);
#endif
                
#endif
                }
                
                currentPartials.push_back(newPartial);
            }
            
            // If DEAD, do not add, forget it
        }
    }
    
    // Get the result here
    // So we get the partials that are well tracked over time
    *partials = currentPartials;
    
    // At the end, there remains the partial that have not been matched
    //
    // Add them at to the history for next time
    //
    for (int i = 0; i < remainingPartials.size(); i++)
    {
        Partial p = remainingPartials[i];
        
        p.GenNewId();
        
        currentPartials.push_back(p);
    }
    
    // Then sort the new partials by frequency
    sort(currentPartials.begin(), currentPartials.end(), Partial::FreqLess);
    
    //
    // Update: add the partials to the history
    // (except the dead ones)
    mPartials[0].clear();
    for (int i = 0; i < currentPartials.size(); i++)
    {
        const Partial &currentPartial = currentPartials[i];

#if 0 // ORIGIN
        // TEST: do not skip the dead partials:
        // they will be used for fade out !
        //if (currentPartial.mState != Partial::DEAD)
        mPartials[0].push_back(currentPartial);
#endif

#if 1
        if (currentPartial.mState != Partial::DEAD)
            mPartials[0].push_back(currentPartial);
#endif
    }

    *partials = mPartials[0];

    // DEBUG
    DBG_PrintPartials(*partials);
    
    BL_FLOAT coeff = 1.0/(mSampleRate*0.5);
    for (int i = 0; i < partials->size(); i++)
    {
        Partial &p = (*partials)[i];
        p.mFreq *= coeff;
        p.mBeta0 *= coeff;

        //#if !EXTRAPOLATE_KALMAN
        //p.mPredictedFreq = p.mFreq;
        //#endif
    }
}

void
PartialFilterAMFM::
AssociatePartialsAMFM(const vector<Partial> &prevPartials,
                      vector<Partial> *currentPartials,
                      vector<Partial> *remainingPartials)
{
    // Sort current partials and prev partials by increasing frequency
    sort(currentPartials->begin(), currentPartials->end(), Partial::FreqLess);
    
    vector<Partial> &prevPartials0 = mTmpPartials5;
    prevPartials0 = prevPartials;
    sort(prevPartials0.begin(), prevPartials0.end(), Partial::FreqLess);
    
    // Associated partials
    bool stopFlag = true;
    do {
        stopFlag = true;
        
        for (int i = 0; i < prevPartials0.size(); i++)
        {
            const Partial &prevPartial = prevPartials0[i];
            for (int j = 0; j < currentPartials->size(); j++)
            {
                Partial &currentPartial = (*currentPartials)[j];
                if (currentPartial.mId != -1)
                    // Already associated, nothing to do on this step!
                    continue;
                
                BL_FLOAT LA = ComputeLA(prevPartial, currentPartial);
                BL_FLOAT LF = ComputeLF(prevPartial, currentPartial);

                //fprintf(stderr, "LA: %g LF: %g\n", LA, LF);

                // TODO: maybe manage the "scores" better
                if ((LA > 0.5) && (LF > 0.5))
                    //if ((LA > 0.7) && (LF > 0.7))
                    // Associate!
                {
                    int otherIdx =
                        FindPartialById(*currentPartials, (int)prevPartial.mId);

                    if (otherIdx == -1)
                        // This partial is not yet associated
                        // => No fight
                    {
                        currentPartial.mId = prevPartial.mId;
                        currentPartial.mAge = prevPartial.mAge;

#if EXTRAPOLATE_KALMAN
                        currentPartial.mKf = prevPartial.mKf; //
#endif
                        
                        stopFlag = false;
                    }
#if 1 //0
                    else // Fight!
                    {
                        Partial &otherPartial = (*currentPartials)[otherIdx];

                        BL_FLOAT otherLA = ComputeLA(prevPartial, otherPartial);
                        BL_FLOAT otherLF = ComputeLF(prevPartial, otherPartial);

                        // Joint likelihood
                        BL_FLOAT j0 = LA*LF;
                        BL_FLOAT j1 = otherLA*otherLF;
                        // TODO: manage better the two scores
                        //if ((LA > otherLA) && (LF > otherLF))
                        if (j0 > j1)
                        // Current partial won
                        {
                            currentPartial.mId = prevPartial.mId;
                            currentPartial.mAge = prevPartial.mAge;

#if EXTRAPOLATE_KALMAN
                            currentPartial.mKf = prevPartial.mKf; //
#endif
                            
                            // Detach the other
                            otherPartial.mId = -1;
                            
                            stopFlag = false;
                        }
                        else
                        // Other partial won
                        {
                            // Just keep it like it is!
                        }
                    }
#endif
                }
            }
        }
    } while (!stopFlag);
    
    
    // Update partials
    vector<Partial> &newPartials = mTmpPartials6;
    newPartials.resize(0);
    
    for (int j = 0; j < currentPartials->size(); j++)
    {
        Partial &currentPartial = (*currentPartials)[j];

        if (currentPartial.mId != -1)
        {
            currentPartial.mState = Partial::ALIVE;
            currentPartial.mWasAlive = true;
    
            // Increment age
            currentPartial.mAge = currentPartial.mAge + 1;

#if 0 // No need
            
#if EXTRAPOLATE_KALMAN
            //currentPartial.mPredictedFreq =
            //currentPartial.mFreq =
            //    currentPartial.mKf.updateEstimate(currentPartial.mFreq);
            ExtrapolatePartialKalman(&currentPartial);
#endif
            
#if EXTRAPOLATE_AMFM
            ExtrapolatePartialAMFM(&currentPartial);
#endif

#endif
            
            newPartials.push_back(currentPartial);
        }
    }
    
    // Add the remaining partials
    remainingPartials->clear();
    for (int i = 0; i < currentPartials->size(); i++)
    {
        const Partial &p = (*currentPartials)[i];
        if (p.mId == -1)
            remainingPartials->push_back(p);
    }
    
    // Update current partials
    *currentPartials = newPartials;
}

int
PartialFilterAMFM::FindPartialById(const vector<Partial> &partials, int idx)
{
    for (int i = 0; i < partials.size(); i++)
    {
        const Partial &partial = partials[i];
        
        if (partial.mId == idx)
            return i;
    }
    
    return -1;
}

// Compute amplitude likelihood
// (increase when the penality decrease)
BL_FLOAT
PartialFilterAMFM::ComputeLA(const Partial &currentPartial,
                             const Partial &otherPartial)
{
#if 0 // Use trapezoid
    // Points
    BL_FLOAT a = currentPartial.mAmp; 
    BL_FLOAT b = currentPartial.mAmp + currentPartial.mAlpha0;
    BL_FLOAT c = otherPartial.mAmp; 
    BL_FLOAT d = otherPartial.mAmp - otherPartial.mAlpha0;

#if 0
    a = BLUtils::DBToAmp(a);
    b = BLUtils::DBToAmp(b);
    c = BLUtils::DBToAmp(c);
    d = BLUtils::DBToAmp(d);
#endif
    
    // Area
    BL_FLOAT area = ComputeTrapezoidArea(a, b, c, d);
#endif

#if 1 // Use general polygon
    BL_FLOAT x[4] = { 0.0, 1.0, 1.0, 0.0 };
    BL_FLOAT y[4];
    if (currentPartial.mAmp < otherPartial.mAmp)
    {
        y[0] = currentPartial.mAmp;
        y[1] = currentPartial.mAmp + currentPartial.mAlpha0;
        y[2] = otherPartial.mAmp;
        y[3] = otherPartial.mAmp - otherPartial.mAlpha0;
    }
    else
    {
        y[0] = otherPartial.mAmp - otherPartial.mAlpha0;
        y[1] = otherPartial.mAmp;
        y[2] = currentPartial.mAmp + currentPartial.mAlpha0;
        y[3] = currentPartial.mAmp;
    }

    BL_FLOAT area = BLUtilsMath::PolygonArea(x, y, 4);
#endif
    
    // u
    BL_FLOAT denom = sqrt(currentPartial.mAmp*otherPartial.mAmp);
    BL_FLOAT ua = 0.0;
    if (denom > BL_EPS)
        ua = area/denom;
    
    // Likelihood
    BL_FLOAT LA = 1.0/(1.0 + ua);
    
    return LA;
}

// Compute frequency likelihood
// (increase when the penality decrease)
BL_FLOAT
PartialFilterAMFM::ComputeLF(const Partial &currentPartial,
                             const Partial &otherPartial)
{
#if 0 // Trapezoid
    // Points
    BL_FLOAT a = currentPartial.mFreq; 
    BL_FLOAT b = currentPartial.mFreq + currentPartial.mBeta0;
    BL_FLOAT c = otherPartial.mFreq; 
    BL_FLOAT d = otherPartial.mFreq - otherPartial.mBeta0;
    
    // Area
    BL_FLOAT area = ComputeTrapezoidArea(a, b, c, d);
#endif
#if 1 // General polygon
    BL_FLOAT x[4] = { 0.0, 1.0, 1.0, 0.0 };
    BL_FLOAT y[4];
    if (currentPartial.mAmp < otherPartial.mAmp)
    {
        y[0] = currentPartial.mFreq;
        y[1] = currentPartial.mFreq + currentPartial.mBeta0;
        y[2] = otherPartial.mFreq;
        y[3] = otherPartial.mFreq - otherPartial.mBeta0;
    }
    else
    {
        y[0] = otherPartial.mFreq - otherPartial.mBeta0;
        y[1] = otherPartial.mFreq;
        y[2] = currentPartial.mFreq + currentPartial.mBeta0;
        y[3] = currentPartial.mFreq;
    }
    
    BL_FLOAT area = BLUtilsMath::PolygonArea(x, y, 4);
#endif
    
    // u
    BL_FLOAT denom = sqrt(currentPartial.mFreq*otherPartial.mFreq);
    BL_FLOAT uf = 0.0;
    if (denom > BL_EPS)
        uf = area/denom;
    
    // Likelihood
    BL_FLOAT LF = 1.0/(1.0 + uf);
        
    return LF;
}

// Trapezoid: https://en.wikipedia.org/wiki/Trapezoid
BL_FLOAT
PartialFilterAMFM::ComputeTrapezoidArea(BL_FLOAT a, BL_FLOAT b,
                                        BL_FLOAT c, BL_FLOAT d)
{
    BL_FLOAT b0 = std::fabs(d - a);
    BL_FLOAT b1 = std::fabs(c - b);

    // Trapezoid height (T = 1)
    BL_FLOAT h = 1.0;

    BL_FLOAT area = (b0 + b1)*0.5*h;
    
    return area;
}

// Extrapolate the partial with alpha0 and beta0
void
PartialFilterAMFM::ExtrapolatePartialAMFM(Partial *p)
{
    // Amp is normalized
    p->mAmp += p->mAlpha0;
    if (p->mAmp < 0.0)
        p->mAmp = 0.0;
    if (p->mAmp > 1.0)
        p->mAmp = 1.0;

    // Freq is real freq
    p->mFreq += p->mBeta0;
    if (p->mFreq < 0.0)
        p->mFreq = 0.0;
    if (p->mFreq > mSampleRate*0.5)
        p->mFreq = mSampleRate*0.5;
}

void
PartialFilterAMFM::ExtrapolatePartialKalman(Partial *p)
{
    p->mFreq /= mSampleRate*0.5;
    
    p->mFreq = p->mKf.updateEstimate(p->mFreq);

    p->mFreq *= mSampleRate*0.5;
}

void
PartialFilterAMFM::DBG_PrintPartials(const vector<Partial> &partials)
{
    fprintf(stderr, "-------------------\n");
    for (int i = 0; i < partials.size(); i++)
    {
        const Partial &p = partials[i];
        
        fprintf(stderr, "id: %ld state: %d amp: %g freq: %g\n",
                p.mId, p.mState, p.mAmp, p.mFreq);
    }
}
