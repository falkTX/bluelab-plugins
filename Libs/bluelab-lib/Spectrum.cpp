//
//  Spectrum.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 16/05/17.
//
//

#include "Spectrum.h"

#ifndef MAX_PATH
//#define MAX_PATH 512
#define MAX_PATH 1024
#endif

Spectrum::Spectrum(int width)
{
    mWidth = width;
    
    mInputMultiplier = 1.0;
}

Spectrum::~Spectrum() {}

void
Spectrum::SetInputMultiplier(BL_FLOAT mult)
{
    mInputMultiplier = mult;
}

Spectrum *
Spectrum::Load(const char *fileName)
{
    // TODO
    return NULL;
}

void
Spectrum::AddLine(const WDL_TypedBuf<BL_FLOAT> &newLine)
{
    if (newLine.GetSize() != mWidth)
    {
        abort();
    }
    
    WDL_TypedBuf<BL_FLOAT> newLineMult;
    for (int i = 0; i < newLine.GetSize(); i++)
        newLineMult.Add(newLine.Get()[i]*mInputMultiplier);
    
    mLines.push_back(newLineMult);
}

const WDL_TypedBuf<BL_FLOAT> *
Spectrum::GetLine(int index)
{
    if (index >= mLines.size())
        return NULL;
    
    return &mLines[index];
}

void
Spectrum::Save(const char *filename)
{
    char fullFilename[MAX_PATH];
    sprintf(fullFilename, "/Volumes/HDD/Share/%s", filename);
    
    FILE *file = fopen(fullFilename, "w");
    
    for (int j = 0; j < mLines.size(); j++)
    {
        const WDL_TypedBuf<BL_FLOAT> &line = mLines[j];
        
        for (int i = 0; i < mWidth; i++)
        {
            fprintf(file, "%f ", line.Get()[i]);
        }
        
        fprintf(file, "\n");
    }
    
    fclose(file);
}
