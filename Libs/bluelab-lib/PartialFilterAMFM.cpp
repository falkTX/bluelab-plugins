#include <stdio.h>

#include <cmath>
#include <algorithm>
using namespace std;

#include <BLUtilsMath.h>

#include "PartialFilterAMFM.h"

#define MAX_ZOMBIE_AGE 2

#define PARTIALS_HISTORY_SIZE 2

PartialFilterAMFM::PartialFilterAMFM(int bufferSize)
{
    mBufferSize = bufferSize;
}

PartialFilterAMFM::~PartialFilterAMFM() {}

void
PartialFilterAMFM::Reset(int bufferSize)
{
    mBufferSize = bufferSize;

    mPartials.clear();
}
           
void
PartialFilterAMFM::FilterPartials(vector<Partial> *partials)
{    
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
                
                // Kalman:
                // GOOD: extrapolate the zombies
                newPartial.mPredictedFreq =
                    newPartial.mKf.updateEstimate(newPartial.mFreq);
                
                currentPartials.push_back(newPartial);
            }
            else if (prevPartial.mState == Partial::ZOMBIE)
            {
                Partial newPartial = prevPartial;
                
                newPartial.mZombieAge++;
                if (newPartial.mZombieAge >= MAX_ZOMBIE_AGE)
                    newPartial.mState = Partial::DEAD;
  
                // Kalman
                // GOOD: extrapolate the zombies
                newPartial.mPredictedFreq =
                    newPartial.mKf.updateEstimate(newPartial.mFreq);

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
        
        // TEST: do not skip the dead partials:
        // they will be used for fade out !
        //if (currentPartial.mState != Partial::DEAD)
        mPartials[0].push_back(currentPartial);
    }

    *partials = mPartials[0];
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
                
                BL_FLOAT scoreAmp = ComputeScoreAmp(prevPartial, currentPartial);
                BL_FLOAT scoreFreq = ComputeScoreFreq(prevPartial, currentPartial);

                // TODO: maybe manage the scores better
                if ((scoreAmp < 0.5) &&
                    (scoreFreq < 0.5))
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
                        currentPartial.mKf = prevPartial.mKf; //
                        
                        stopFlag = false;
                    }
                    else // Fight!
                    {
                        Partial &otherPartial = (*currentPartials)[otherIdx];

                        BL_FLOAT otherScoreAmp =
                            ComputeScoreAmp(prevPartial, otherPartial);
                        BL_FLOAT otherScoreFreq =
                            ComputeScoreFreq(prevPartial, otherPartial);
                
                        // TODO: manage better the two scores
                        if ((scoreAmp < otherScoreAmp) &&
                            (scoreFreq < otherScoreFreq))
                        // Current partial won
                        {
                            currentPartial.mId = prevPartial.mId;
                            currentPartial.mAge = prevPartial.mAge;
                            currentPartial.mKf = prevPartial.mKf; //
                            
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
            currentPartial.mPredictedFreq =
                    currentPartial.mKf.updateEstimate(currentPartial.mFreq);
    
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

BL_FLOAT
PartialFilterAMFM::ComputeScoreAmp(const Partial &currentPartial,
                                   const Partial &otherPartial)
{
    BL_FLOAT a = currentPartial.mAmp; 
    BL_FLOAT b = currentPartial.mAmp + currentPartial.mAlpha0;
    BL_FLOAT c = otherPartial.mAmp; 
    BL_FLOAT d = otherPartial.mAmp - otherPartial.mAlpha0;

    BL_FLOAT area = ComputeArea(a, b, c, d);

    BL_FLOAT denom = sqrt(currentPartial.mAmp*otherPartial.mAmp);
    BL_FLOAT score = 0.0;
    if (denom > BL_EPS)
        score = score/denom;

    return score;
}

BL_FLOAT
PartialFilterAMFM::ComputeScoreFreq(const Partial &currentPartial,
                                    const Partial &otherPartial)
{
    BL_FLOAT a = currentPartial.mFreq; 
    BL_FLOAT b = currentPartial.mFreq + currentPartial.mBeta0;
    BL_FLOAT c = otherPartial.mFreq; 
    BL_FLOAT d = otherPartial.mFreq - otherPartial.mBeta0;

    BL_FLOAT area = ComputeArea(a, b, c, d);

    BL_FLOAT denom = sqrt(currentPartial.mFreq*otherPartial.mFreq);
    BL_FLOAT score = 0.0;
    if (denom > BL_EPS)
        score = score/denom;

    return score;
}

BL_FLOAT
PartialFilterAMFM::ComputeArea(BL_FLOAT a, BL_FLOAT b, BL_FLOAT c, BL_FLOAT d)
{
    // TODO
    return -1.0;
}
