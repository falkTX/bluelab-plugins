#include <math.h>

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
    
    vector<BL_FLOAT> splitCurve;
    splitCurve.resize(magns.size());

    // Multi-resolution dichotomic search
    // First, take an horizontal line, covering all
    // the frequencies... and choose its best height in amp
    // Then take the middle point of the line,
    // ... and choose its best height in amp
    // and so on until we get a good resolution

#define RESOLUTION 64 //1
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
        
        for (int j = i; j < i + step; j++)
            splitCurve[j] = res;
    }

    *sig = splitCurve;
    noise->resize(sig->size());
    for (int i = 0; i < noise->size(); i++)
        (*noise)[i] = magns[i] - (*sig)[i];
}

// Normalized spectral irregularity
BL_FLOAT
SISplitter::computeSpectralIrreg(const vector<BL_FLOAT> &magns,
                                 int startI, int endBin)
{
    if (magns.empty())
        return 0.0;

    BL_FLOAT irreg = 0.0;
    BL_FLOAT norm = 0.0;
    BL_FLOAT prevMagn = magns[0];
    for (int i = startI + 1; i <= endBin; i++)
    {
        BL_FLOAT m = magns[i];
        norm += m*m;
        
        BL_FLOAT diff = m - prevMagn;
        irreg += diff*diff;
    }

    if (norm > EPS)
        irreg /= norm;

    return irreg;
}

BL_FLOAT
SISplitter::computeScore(const vector<BL_FLOAT> &magns,
                         BL_FLOAT refMagn,
                         int startBin, int endBin)
{
    vector<BL_FLOAT> magns0 = magns;
    for (int i = 0; i < magns0.size(); i++)
        magns0[i] = magns0[i] - refMagn;
    BL_FLOAT ir0 = computeSpectralIrreg(magns, startBin, endBin);

    BL_FLOAT minMagn = 0.0;
    BL_FLOAT maxMagn = 1.0;
    findMinMax(magns, startBin, endBin, &minMagn, &maxMagn);
    
    vector<BL_FLOAT> magns1 = magns;
    for (int i = 0; i < magns1.size(); i++)
        magns1[i] = magns1[i] - (maxMagn - refMagn);
    BL_FLOAT ir1 = computeSpectralIrreg(magns1, startBin, endBin);
    
    BL_FLOAT score = ir0 + (1.0 - ir1);

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

