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
