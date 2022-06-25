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
//  OnsetDetector.cpp
//  BL-OnsetDetect
//
//  Created by applematuer on 8/2/20.
//
//

#include <math.h>

#include <vector>
#include <algorithm> // for sort()
using namespace std;

#include "OnsetDetector.h"

OnsetDetector::OnsetDetector()
{
    mThreshold = 0.94;
    
    mCurrentOnsetValue = 0.0;
}

OnsetDetector::~OnsetDetector() {}

void
OnsetDetector::SetThreshold(BL_FLOAT threshold)
{
    mThreshold = threshold;
}

void
OnsetDetector::Detect(const WDL_TypedBuf<BL_FLOAT> &magns)
{
    // Sort the magnitude spectrum coefficients in each frame Xik in ascending order
    vector<BL_FLOAT> magnsVec;
    magnsVec.resize(magns.GetSize());
    for (int i = 0; i < magns.GetSize(); i++)
    {
        BL_FLOAT val = magns.Get()[i];
        magnsVec[i] = val;
    }
    
    sort(magnsVec.begin(), magnsVec.end());
    
    //  Use only the first J out of N coefficients
    int J = mThreshold*magns.GetSize();
    
    magnsVec.resize(J);
    
    // Compute the INOS ODF (gamma)
    // (two steps)
    
    // 1 - First compute sum(X^2)
    BL_FLOAT sumX2 = 0.0;
    for (int i = 0; i < magnsVec.size(); i++)
    {
        BL_FLOAT val = magnsVec[i];
        BL_FLOAT val2 = val*val;
        
        sumX2 += val2;
    }
    
    // 2 - Then compute (sum(X^4))^1/4
    BL_FLOAT sumX4 = 0.0;
    for (int i = 0; i < magnsVec.size(); i++)
    {
        BL_FLOAT val = magnsVec[i];
        BL_FLOAT val2 = val*val;
        BL_FLOAT val4 = val2*val2;
        
        sumX4 += val4;
    }
    
    BL_FLOAT denom = std::pow(sumX4, 0.25);
    
    // Finally compute gamma = sumX2/denom
    BL_FLOAT sigma = 0.0;
    if (denom > 1e-15)
        sigma = sumX2/denom;
    
    mCurrentOnsetValue = sigma;
}

BL_FLOAT
OnsetDetector::GetCurrentOnsetValue()
{
    return mCurrentOnsetValue;
}
