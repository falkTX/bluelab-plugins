#include <stdio.h>

#include <cmath>
#include <algorithm>
using namespace std;

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include "PartialFilterAMFM.h"

#define MAX_ZOMBIE_AGE 3 //2 //5 //2
#define PARTIALS_HISTORY_SIZE 3 //2


#define EXTRAPOLATE_KALMAN 0 // 1
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
    vector<Partial> &remainingCurrentPartials = mTmpPartials1;
    remainingCurrentPartials.resize(0);
    
    AssociatePartialsAMFM(prevPartials, &currentPartials, &remainingCurrentPartials);

    vector<Partial> &deadZombiePartials = mTmpPartials7;
    ComputeZombieDeadPartials(prevPartials, currentPartials, &deadZombiePartials);
    
    // Add zombie and dead partial
    for (int i = 0; i < deadZombiePartials.size(); i++)
        currentPartials.push_back(deadZombiePartials[i]);

    //FixPartialsCrossing(prevPartials, &currentPartials);
    if (mPartials.size() >= 3)
        FixPartialsCrossing(mPartials[2], mPartials[1], &currentPartials);
    
    // Get the result here
    // So we get the partials that are well tracked over time
    //*partials = currentPartials;
    
    // At the end, there remains the partial that have not been matched
    //
    // Add them at to the history for next time
    //
    for (int i = 0; i < remainingCurrentPartials.size(); i++)
    {
        Partial p = remainingCurrentPartials[i];
        
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
    //DBG_PrintPartials(*partials);
    
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
                      vector<Partial> *remainingCurrentPartials)
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
    remainingCurrentPartials->clear();
    for (int i = 0; i < currentPartials->size(); i++)
    {
        const Partial &p = (*currentPartials)[i];
        if (p.mId == -1)
            remainingCurrentPartials->push_back(p);
    }
    
    // Update current partials
    *currentPartials = newPartials;
}

void
PartialFilterAMFM::ComputeZombieDeadPartials(const vector<Partial> &prevPartials,
                                             const vector<Partial> &currentPartials,
                                             vector<Partial> *zombieDeadPartials)
{
    zombieDeadPartials->clear();
    
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
                //if (newPartial.mZombieAge < MAX_ZOMBIE_AGE)
                //    currentPartials.push_back(newPartial);

                zombieDeadPartials->push_back(newPartial);
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
                
                zombieDeadPartials->push_back(newPartial);
            }
            
            // If DEAD, do not add, forget it
        }
    }
}

#if 0 // First test
// Simple fix for partial crossing error
void
PartialFilterAMFM::FixPartialsCrossing(const vector<Partial> &prevPartials,
                                       vector<Partial> *currentPartials)
{    
    // 100Hz
#define FREQ_THRESHOLD 100.0

    fprintf(stderr, "----------------------\n");
    fprintf(stderr, "num: %d %d\n", prevPartials.size(), currentPartials->size());
        
    const vector<Partial> currentPartials0 = *currentPartials;
        
    Partial p0[2];
    Partial p1[2];
    for (int i = 0; i < currentPartials0.size(); i++)
    {
        p0[1] = currentPartials0[i];
        if (p0[1].mId == -1)
            continue;
        
        bool found0 = false;
        for (int j = 0; j < prevPartials.size(); j++)
        {
            const Partial &p = prevPartials[j];
            if (p.mId == p0[1].mId)
            {
                p0[0] = p;
                
                found0 = true;
                
                break;
            }
        }

        if (!found0)
            continue;

        for (int j = i + 1/*0*/; j < currentPartials0.size(); j++)
        {
            //if (j == i)
            //    continue;
            
            const Partial &p = currentPartials0[j];
            p1[1] = p;
            if (p1[1].mId == -1)
                continue;
            
            bool found1 = false;
            for (int k = 0; k < prevPartials.size(); k++)
            {
                const Partial &p = prevPartials[k];
                if (p.mId == p1[1].mId)
                {
                    p1[0] = p;
                    
                    found1 = true;
                    
                    break;
                }
            }

            if (!found1)
                continue;

            // Frequencies too far
            if (std::fabs(p0[0].mFreq - p1[0].mFreq) > FREQ_THRESHOLD)
                continue;
            if (std::fabs(p0[1].mFreq - p1[1].mFreq) > FREQ_THRESHOLD)
                continue;

            // Test if frequancies are crossing
            //if ((p0[1].mFreq - p1[0].mFreq)*(p1[1].mFreq - p0[0].mFreq) > 0.0)
            BL_FLOAT seg0[2][2] = { { p0[1].mFreq, 0.0 }, { p0[0].mFreq, 1.0 } };
            BL_FLOAT seg1[2][2] = { { p1[1].mFreq, 0.0 }, { p1[0].mFreq, 1.0 } };
            bool intersect = BLUtilsMath::SegSegIntersect2(seg0, seg1);

            fprintf(stderr, "intersect: %d\n", intersect);
            
            if (!intersect)
                // Freqs are not crossing => must swap
            {
                fprintf(stderr, "swap! %d %d\n", p0[1].mId, p1[1].mId);
                
                int tmpId = p0[1].mId;
                p0[1].mId = p1[1].mId;
                p1[1].mId = tmpId;
                
                (*currentPartials)[i].mId = p0[1].mId;
                (*currentPartials)[j].mId = p1[1].mId;
            }
        }
    }
}
#endif

// Simple fix for partial crossing error
void
PartialFilterAMFM::FixPartialsCrossing(const vector<Partial> &partials0,
                                       const vector<Partial> &partials1,
                                       vector<Partial> *partials2)
{    
    //fprintf(stderr, "----------------------\n");
    //fprintf(stderr, "num: %d %d\n", partials1.size(), partials2->size());
        
    const vector<Partial> partials2Copy = *partials2;
        
    Partial p0[3];
    Partial p1[3];
    
    for (int i = 0; i < partials2Copy.size(); i++)
    {
        p0[2] = partials2Copy[i];
        if (p0[2].mId == -1)
            continue;

        int idx01 = FindPartialById(partials1, p0[2].mId);
        if (idx01 == -1)
            continue;
        p0[1] = partials1[idx01];

        int idx00 = FindPartialById(partials0, p0[2].mId);
        if (idx00 == -1)
            continue;
        p0[0] = partials0[idx00];

        //
        for (int j = i + 1; j < partials2Copy.size(); j++)
        {
            const Partial &p = partials2Copy[j];
            p1[2] = p;
            if (p1[2].mId == -1)
                continue;

            int idx11 = FindPartialById(partials1, p1[2].mId);
            if (idx11 == -1)
                continue;
            p1[1] = partials1[idx11];
            
            int idx10 = FindPartialById(partials0, p1[2].mId);
            if (idx10 == -1)
                continue;
            p1[0] = partials0[idx10];

            // Extrapolated values
            BL_FLOAT extraP0 = p0[1].mFreq + (p0[1].mFreq - p0[0].mFreq);
            BL_FLOAT extraP1 = p1[1].mFreq + (p1[1].mFreq - p1[0].mFreq);

            // Check if extrapolated points intersect
            BL_FLOAT extraSeg0[2][2] = { { p0[1].mFreq, 0.0 }, { extraP0, 1.0 } };
            BL_FLOAT extraSeg1[2][2] = { { p1[1].mFreq, 0.0 }, { extraP1, 1.0 } };
            bool extraIntersect = BLUtilsMath::SegSegIntersect2(extraSeg0, extraSeg1);

            // Check if real points intersect
            BL_FLOAT seg0[2][2] = { { p0[1].mFreq, 0.0 }, { p0[2].mFreq, 1.0 } };
            BL_FLOAT seg1[2][2] = { { p1[1].mFreq, 0.0 }, { p1[2].mFreq, 1.0 } };
            bool intersect = BLUtilsMath::SegSegIntersect2(seg0, seg1);
            
            if (intersect != extraIntersect)
            {
                //fprintf(stderr, "swap! %d %d\n", p0[1].mId, p1[1].mId);
                
                int tmpId = p0[2].mId;
                p0[2].mId = p1[2].mId;
                p1[2].mId = tmpId;
                
                (*partials2)[i].mId = p0[2].mId;
                (*partials2)[j].mId = p1[2].mId;
            }
        }
    }
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
