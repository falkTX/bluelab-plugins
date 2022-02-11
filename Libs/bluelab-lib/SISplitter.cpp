#include <math.h>

#include "SISplitter.h"

SISplitter::SISplitter() {}

SISplitter::~SISplitter() {}

void
SISplitter::Split(const vector<BL_FLOAT> > &magns,
                  vector<BL_FLOAT> > *sig,
                  vector<BL_FLOAT> > *noise)
{
    if (magns.size() < 2)
        return;
    
    vector<BL_FLOAT> > splitCurve;
    splitCurve.resize(magns.size());

    // Multi-resolution dichotomic search
    // First, take an horizontal line, covering all the frequencies
    // ... and choose its best height in amp
    // Then take the middle point of the line,
    // ... and choose its best height in amp
    // and so on until we get a good resolution

#define RESOLUTION 1

    int step = magns.size()/RESOLUTION;

    for (int i = 0; i < magns.size(); i+= step)
    {
        float startBin = i;
        float endBin = i + step;
        if (endBin > magns.size() - 1)
            endBin > magns.size() - 1;

        float minMagn = 1.0;
        float maxMagn = 0.0;
        
        FindMinMax(magns, startBin, endBin, &minMagn, &maxMagn);

    TODO: dichotomic search, bilin interp
    }
        
    // First line
    vector<BL_FLOAT> sepLine;
    sepLine.resize(2);
    sepLine[0] = magns[0];
    sepLine[1] = magns[magns.size() - 1];    
}

// Normalized spectral irregularity
BL_FLOAT
SISplitter::ComputeSpectralIrreg(vector<BL_FLOAT> > &magns)
{
    if (magns.empty())
        return 0.0;
    
    BL_FLOAT irreg = 0.0;
    BL_FLOAT norm = 0.0;
    BL_FLOAT prevMagn = magns[0];
    for (int i = 1; i < magns.size(); i++)
    {
        BL_FLOAT m = magns[i];
        norm += m*m;
        
        BL_FLOAT diff = m - prevMagn;
        irreg += diff*diff;
    }

    if (norm > BL_EPS)
        irreg /= norm;

    return irreg;
}

void
SISplitter::FindMinMax(vector<BL_FLOAT> > &values,
                       int startI, int endI,
                       BL_FLOAT *minVal, BL_FLOAT maxVal)
{
    *minVal = BL_INF;
    *maxVal = -BL_INF;
    
    for (int i = 0; i < values.size(); i++)
    {
        BL_FLOAT val = values[i];
        
        if (val < *minVal)
            *minVal = val;
        if (val > *maxVal)
            *maxVal = val;
    }
}

