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
//  KalmanParamSmooth.cpp
//  UST
//
//  Created by applematuer on 2/29/20.
//
//

#include "KalmanParamSmooth.h"

KalmanParamSmooth::KalmanParamSmooth(BL_FLOAT samplingRate)
{
    mSampleRate = samplingRate;
    
    mMea = DEFAULT_KF_E_MEA;
    mEst = DEFAULT_KF_E_EST;
    mQ = DEFAULT_KF_Q;
    
    BL_FLOAT coeff = 44100.0/mSampleRate;
    
    mKf = new SimpleKalmanFilter(mMea*coeff, mEst*coeff, mQ);
    
    mInitialized = false;
}

KalmanParamSmooth::~KalmanParamSmooth()
{
    delete mKf;
}

void
KalmanParamSmooth::Reset(BL_FLOAT samplingRate)
{
    mSampleRate = samplingRate;
    
    SetKalmanParameters(mMea, mEst, mQ);
    
    mInitialized = false;
}

void
KalmanParamSmooth::Reset(BL_FLOAT samplingRate, BL_FLOAT val)
{
    mSampleRate = samplingRate;
    
    SetKalmanParameters(mMea, mEst, mQ);
    
    mKf->initEstimate(val);
    mInitialized = true;
}

void
KalmanParamSmooth::SetKalmanParameters(BL_FLOAT mea, BL_FLOAT est, BL_FLOAT q)
{
    mMea = mea;
    mEst = est;
    mQ = q;
    
    BL_FLOAT coeff = 44100.0/mSampleRate;
    
    delete mKf;
    mKf = new SimpleKalmanFilter(mMea*coeff, mEst*coeff, mQ);
}

BL_FLOAT
KalmanParamSmooth::Process(BL_FLOAT inVal)
{
    if (!mInitialized)
    {
        mKf->initEstimate(inVal);
        
        mInitialized = true;
        
        return inVal;
    }
    
    BL_FLOAT result = mKf->updateEstimate(inVal);
    
    return result;
}
