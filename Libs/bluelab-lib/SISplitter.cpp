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
 
#include <math.h>

#include <BLDebug.h>

#include "SISplitter.h"

#define EPS 1e-15
#define INF 1e15

// seems to change nothing...
#define MAKE_DIFF 0

#define MIN_RESO 4
#define MAX_RESO 512 //64

#define DICHO_EPS 0.001 // ~8steps
//#define DICHO_EPS 0.01 // ~4steps
//#define DICHO_EPS 0.0001 // ~16 steps?

#define SPECTRAL_IRREG_METHOD computeSpectralIrreg_J

SISplitter::SISplitter()
{
    _reso = MIN_RESO;
}

SISplitter::~SISplitter() {}

void
SISplitter::split(const vector<BL_FLOAT> &magns,
                  vector<BL_FLOAT> *sig,
                  vector<BL_FLOAT> *noise)
{
    if (magns.size() < 2)
        return;
    
    vector<BL_FLOAT> splitCurve;
    splitCurve.resize(magns.size());

    // Multi-resolution dichotomic search
    // First, take an horizontal line, covering all
    // the frequencies... and choose its best height in amp
    // Then take the middle point of the line,
    // ... and choose its best height in amp
    // and so on until we get a good resolution

    // dbg
    int avgNumDichoSteps = 0;
    
    int step = magns.size()/_reso;
    for (int i = 0; i < magns.size(); i+= step)
    {
        BL_FLOAT startBin = i;
        BL_FLOAT endBin = i + step;
        if (endBin > magns.size() - 1)
            endBin > magns.size() - 1;

        BL_FLOAT minMagn = 0.0;
        BL_FLOAT maxMagn = 1.0;
        findMinMax(magns, startBin, endBin, &minMagn, &maxMagn);

        BL_FLOAT delta = maxMagn - minMagn;
        while(delta > DICHO_EPS)
        {
            avgNumDichoSteps++;
            
            BL_FLOAT midMagn = (minMagn + maxMagn)*0.5;

            delta = fabs(maxMagn - minMagn);
            
            BL_FLOAT s0 =
                computeScore(magns, minMagn, startBin, endBin);
            BL_FLOAT s1 =
                computeScore(magns, midMagn, startBin, endBin);
            BL_FLOAT s2 =
                computeScore(magns, maxMagn, startBin, endBin);

            if (fabs(s1 - s0) < (fabs(s2 - s1)))
                maxMagn = midMagn;
            else
                minMagn = midMagn;
        }

        BL_FLOAT res = (minMagn + maxMagn)*0.5;

        // fill
        if (i == 0)
        {
            // flat
            for (int j = i; j < i + step; j++)
                splitCurve[j] = res;
        }
        else
        {
            // linerp
            BL_FLOAT prev = splitCurve[i - 1];
            for (int j = i; j < i + step; j++)
            {
                BL_FLOAT t = ((BL_FLOAT)j - i)/(step - 1);
                
                splitCurve[j] = (1.0 - t)*prev + t*res;
            }
        }
    }

    //fprintf(stderr, "avg # steps: %d\n",
    //x        avgNumDichoSteps/(magns.size()/step));

    // split sig and noise
#if !MAKE_DIFF
    *noise = splitCurve;
    sig->resize(noise->size());
    for (int i = 0; i < sig->size(); i++)
    {
        (*sig)[i] = magns[i] - (*noise)[i];
        if ((*sig)[i] < 0.0)
            (*sig)[i] = 0.0;
    }
#else
    //*noise = splitCurve;
    sig->resize(magns.size());
    for (int i = 0; i < noise->size(); i++)
        (*sig)[i] = 0.0;
    
    noise->resize(magns.size());
    for (int i = 0; i < noise->size(); i++)
        (*noise)[i] = 0.0;
    
    for (int i = 0; i < magns.size(); i++)
    {
        if (magns[i] < splitCurve[i])
            (*noise)[i] = magns[i];
        else
            (*noise)[i] = splitCurve[i];
        
        (*sig)[i] = magns[i] - (*noise)[i];
        if ((*sig)[i] < 0.0)
            (*sig)[i] = 0.0;
    }
#endif
}

void
SISplitter::setResolution(int reso)
{
    _reso = reso;

    if (_reso < MIN_RESO)
        _reso = MIN_RESO;
    if (_reso > MAX_RESO)
        _reso = MAX_RESO;
}

// Normalized spectral irregularity
// Jensen, 1999
BL_FLOAT
SISplitter::computeSpectralIrreg_J(const vector<BL_FLOAT> &magns,
                                   int startBin, int endBin)
{
    // sci: sum(diff(magngs)^2)/sum(magns(1:1023)^2)
    if (magns.empty())
        return 0.0;

    BL_FLOAT sumDiff2 = 0.0;
    BL_FLOAT norm2 = 0.0;
    BL_FLOAT prevMagn = magns[startBin];
    for (int i = startBin + 1; i <= endBin; i++)
    {
        BL_FLOAT m = magns[i];
        norm2 += m*m;
        
        BL_FLOAT diff = m - prevMagn;
        sumDiff2 += diff*diff;

        prevMagn = m;
    }

    BL_FLOAT irreg = 0.0;
    if (norm2 > EPS)
        irreg = sumDiff2/norm2;
    
    return irreg;
}

// Spectral irregularity
// Krimphoff et al., 1994
BL_FLOAT
SISplitter::computeSpectralIrreg_K(const vector<BL_FLOAT> &magns,
                                   int startBin, int endBin)
{
    if (magns.size() < 3)
        return 0.0;

    BL_FLOAT sum = 0.0;
    BL_FLOAT denomInv = 1.0/3.0;
    for (int i = startBin + 2; i <= endBin - 1; i++)
    {
        BL_FLOAT m = magns[i];
        BL_FLOAT mn1 = magns[i - 1];
        BL_FLOAT mp1 = magns[i +  1];

        sum += fabs(m - (mn1 + m + mp1)*denomInv); 
    }
    
    BL_FLOAT irreg = sum;
    
    return irreg;
}

void
SISplitter::computeSpectralIrregWin(const vector<BL_FLOAT> &magns,
                                    vector<BL_FLOAT> *siWin,
                                    int winSize, int overlap)
{
    int step = winSize/overlap;
    for (int i = 0; i < magns.size(); i += step)
    {
        int endBin = i + winSize;
        if (endBin > magns.size() - 1)
            endBin = magns.size() - 1;

        // compute SI
        BL_FLOAT si = SPECTRAL_IRREG_METHOD(magns, i, endBin);

        // fill
        if (i == 0)
        {
            // flat
            for (int j = i; j < i + step; j++)
                (*siWin)[j] = si;
        }
        else
        {
            // linerp
            BL_FLOAT prev = (*siWin)[i - 1];
            for (int j = i; j < i + step; j++)
            {
                BL_FLOAT t = ((BL_FLOAT)j - i)/(step - 1);
                
                (*siWin)[j] = (1.0 - t)*prev + t*si;
            }
        }
    }
}

BL_FLOAT
SISplitter::computeScore(const vector<BL_FLOAT> &magns,
                         BL_FLOAT refMagn,
                         int startBin, int endBin)
{
    vector<BL_FLOAT> magns0 = magns;
    for (int i = 0; i < magns0.size(); i++)
        magns0[i] = magns0[i] - refMagn;
    BL_FLOAT si0 = SPECTRAL_IRREG_METHOD(magns, startBin, endBin);

    BL_FLOAT minMagn = 0.0;
    BL_FLOAT maxMagn = 1.0;
    findMinMax(magns, startBin, endBin, &minMagn, &maxMagn);
    
    vector<BL_FLOAT> magns1 = magns;
    for (int i = 0; i < magns1.size(); i++)
        magns1[i] = magns1[i] - (maxMagn - refMagn);
    BL_FLOAT si1 = SPECTRAL_IRREG_METHOD(magns1, startBin, endBin);
    
    BL_FLOAT score = si0 + (1.0 - si1);

    return score;
}

void
SISplitter::findMinMax(const vector<BL_FLOAT> &values,
                       int startBin, int endBin,
                       BL_FLOAT *minVal, BL_FLOAT *maxVal)
{
    *minVal = INF;
    *maxVal = -INF;
    
    for (int i = 0; i < values.size(); i++)
    {
        BL_FLOAT val = values[i];
        
        if (val < *minVal)
            *minVal = val;
        if (val > *maxVal)
            *maxVal = val;
    }
}
