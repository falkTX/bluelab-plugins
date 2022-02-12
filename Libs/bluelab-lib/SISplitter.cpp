#include <math.h>

#include <BLDebug.h>

#include "SISplitter.h"

#define EPS 1e-15
#define INF 1e15

SISplitter::SISplitter() {}

SISplitter::~SISplitter() {}

void
SISplitter::split(const vector<BL_FLOAT> &magns,
                  vector<BL_FLOAT> *sig,
                  vector<BL_FLOAT> *noise)
{
    if (magns.size() < 2)
        return;

#if 0 //1 // debug
    // magns
    BLDebug::DumpData("magns.txt", magns);

    // sci: sum(diff(magns)^2)/sum(magns(1:1023)^2)
    //BL_FLOAT jbSI = computeSpectralIrreg(magns, 0, magns.size() - 1);
    //BLDebug::DumpValue("jb-si.txt", jbSI);
        
    // display SI
    vector<BL_FLOAT> siWin;
    siWin.resize(magns.size());

    // break here!
    
    computeSpectralIrregWin(magns, &siWin, 16/*256*/, 4);
    BLDebug::DumpData("si-win.txt", siWin);
#endif
    
    vector<BL_FLOAT> splitCurve;
    splitCurve.resize(magns.size());

    // Multi-resolution dichotomic search
    // First, take an horizontal line, covering all
    // the frequencies... and choose its best height in amp
    // Then take the middle point of the line,
    // ... and choose its best height in amp
    // and so on until we get a good resolution

    // 256 to be "smooth"?
#define RESOLUTION 256 //64 //1
    int step = magns.size()/RESOLUTION;
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
        while(delta > 0.001)
        {
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
    
    *sig = splitCurve;
    noise->resize(sig->size());
    for (int i = 0; i < noise->size(); i++)
    {
        (*noise)[i] = magns[i] - (*sig)[i];
        if ((*noise)[i] < 0.0)
            (*noise)[i] = 0.0;
    }

#if 0
    BLDebug::DumpData("sig.txt", *sig);
    BLDebug::DumpData("noise.txt", *noise);
#endif
}

// Normalized spectral irregularity
BL_FLOAT
SISplitter::computeSpectralIrreg(const vector<BL_FLOAT> &magns,
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
        BL_FLOAT si = computeSpectralIrreg(magns, i, endBin);

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
    BL_FLOAT si0 = computeSpectralIrreg(magns, startBin, endBin);

    BL_FLOAT minMagn = 0.0;
    BL_FLOAT maxMagn = 1.0;
    findMinMax(magns, startBin, endBin, &minMagn, &maxMagn);
    
    vector<BL_FLOAT> magns1 = magns;
    for (int i = 0; i < magns1.size(); i++)
        magns1[i] = magns1[i] - (maxMagn - refMagn);
    BL_FLOAT si1 = computeSpectralIrreg(magns1, startBin, endBin);
    
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
