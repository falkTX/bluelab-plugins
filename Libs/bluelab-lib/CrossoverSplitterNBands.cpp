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
//  CrossoverSplitterNBands.cpp
//  UST
//
//  Created by applematuer on 8/24/19.
//
//


#include "CrossoverSplitterNBands.h"

#include "so_apf.h"
#include "so_linkwitz_riley_hpf.h"
#include "so_linkwitz_riley_lpf.h"
#include "so_apf2.h"

#define APF SO_APF2
//#define APF SO_APF

CrossoverSplitterNBands::CrossoverSplitterNBands(int numBands,
                                                 BL_FLOAT cutoffFreqs[],
                                                 BL_FLOAT sampleRate)
{
    mNumBands = numBands;
    
    mSampleRate = sampleRate;
    
    if (numBands > 1)
    {
        mFilterChains.resize(mNumBands);
        
        mCutoffFreqs.resize(mNumBands - 1);
        for (int i = 0; i < mNumBands - 1; i++)
            mCutoffFreqs[i] = cutoffFreqs[i];
        
        CreateFilters(mSampleRate);
    }
    
#if OPTIM_AVOID_NEW
    mTmpResultCross = new BL_FLOAT[mNumBands];
    mTmpResultCross2 = new BL_FLOAT[mNumBands];
#endif
}

CrossoverSplitterNBands::~CrossoverSplitterNBands()
{
    for (int i = 0; i < mNumBands; i++)
    {
        //int numFilters = mNumBands - 1;
        int chainSize = mNumBands - i;
        if (i == 0)
            chainSize = mNumBands - 1 - i;
        
        for (int j = 0; j < chainSize; j++)
        {
            if ((i == 0) && (j == 0))
                // First chain, first filter
            {
                // LPF
                SO_LINKWITZ_RILEY_BPF *filter = (SO_LINKWITZ_RILEY_BPF *)mFilterChains[i][j];
                
                delete filter;
            }
            else if ((i > 0) && (j == 0))
                // Other chains, first filter
            {
                // HPF
                SO_LINKWITZ_RILEY_HPF *filter = (SO_LINKWITZ_RILEY_HPF *)mFilterChains[i][j];
                
                delete filter;
            }
            else // (j > 0)
                // Next filters of the chain
            {
                // APF
                APF *filter = (APF *)mFilterChains[i][j];
                
                delete filter;
            }
        }
    }
    
#if OPTIM_AVOID_NEW
    delete mTmpResultCross;
    delete mTmpResultCross2;
#endif
}

void
CrossoverSplitterNBands::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    SetFiltersValues();
}

void
CrossoverSplitterNBands::SetCutoffFreqs(BL_FLOAT freqs[])
{
    for (int j = 0; j < mNumBands - 1; j++)
        mCutoffFreqs[j] = freqs[j];
    
    SetFiltersValues();
}

int
CrossoverSplitterNBands::GetNumBands()
{
    return mNumBands;
}

BL_FLOAT
CrossoverSplitterNBands::GetCutoffFreq(int freqNum)
{
    if (freqNum >= mNumBands - 1)
        return -1.0;
    
    BL_FLOAT freq = mCutoffFreqs[freqNum];
    
    return freq;
}

void
CrossoverSplitterNBands::SetCutoffFreq(int freqNum, BL_FLOAT freq)
{
    if (freqNum >= mNumBands - 1)
        return;
    
    mCutoffFreqs[freqNum] = freq;
    
    // Not optimized, set the values for all the filters
    // (and not only the ones with cutoff)
    SetFiltersValues();
}

void
CrossoverSplitterNBands::Split(BL_FLOAT sample, BL_FLOAT result[])
{
#if !OPTIM_AVOID_NEW
    BL_FLOAT *resultCross = new BL_FLOAT[mNumBands];
#endif
    
    for (int i = 0; i < mFilterChains.size(); i++)
    {
        // First chain
        if (i == 0)
        {
            for (int j = 0; j < mFilterChains[i].size(); j++)
            {
                if (j == 0)
                {
#if !OPTIM_AVOID_NEW
                    resultCross[i] = sample;
#else
                    mTmpResultCross[i] = sample;
#endif
                    
                    SO_LINKWITZ_RILEY_BPF *filter = (SO_LINKWITZ_RILEY_BPF *)mFilterChains[i][j];
                    result[i] = filter->filter(sample);
                }
                else
                {
                    APF *filter = (APF *)mFilterChains[i][j];
                    result[i] = filter->filter(result[i]);
                }
            }
        }
        else // Other chains
        {
            for (int j = 0; j < mFilterChains[i].size(); j++)
            {
                if (j == 0)
                {
                    SO_LINKWITZ_RILEY_HPF *filter = (SO_LINKWITZ_RILEY_HPF *)mFilterChains[i][j];

#if !OPTIM_AVOID_NEW
                    result[i] = filter->filter(resultCross[i - 1]);
                    resultCross[i] = result[i];
#else
                    result[i] = filter->filter(mTmpResultCross[i - 1]);
                    mTmpResultCross[i] = result[i];
#endif
                }
                else if (j == 1)
                {
                    SO_LINKWITZ_RILEY_BPF *filter = (SO_LINKWITZ_RILEY_BPF *)mFilterChains[i][j];
                    result[i] = filter->filter(result[i]);
                }
                else
                {
                    APF *filter = (APF *)mFilterChains[i][j];
                    result[i] = filter->filter(result[i]);
                }
            }
        }
    }
    
#if !OPTIM_AVOID_NEW
    delete resultCross;
#endif
    
    // Invert even signals
    for (int j = 1; j < mNumBands; j += 2)
    {
        result[j] = -result[j];
    }
}

void
CrossoverSplitterNBands::Split(const WDL_TypedBuf<BL_FLOAT> &samples,
                               WDL_TypedBuf<BL_FLOAT> result[])
{
    for (int i = 0; i < mNumBands; i++)
    {
        result[i].Resize(samples.GetSize());
    }
    
#if OPTIM_AVOID_NEW
    BL_FLOAT *r = new BL_FLOAT[mNumBands];
#endif
    
    for (int i = 0; i < samples.GetSize(); i++)
    {
        BL_FLOAT s = samples.Get()[i];
        
#if !OPTIM_AVOID_NEW
        Split(s, r);
#else
        Split(s, mTmpResultCross2);
#endif
        
        for (int k = 0; k < mNumBands; k++)
        {
#if !OPTIM_AVOID_NEW
            result[k].Get()[i] = r[k];
#else
            result[k].Get()[i] = mTmpResultCross2[k];
#endif
        }
    }
    
#if OPTIM_AVOID_NEW
    delete r;
#endif
}

void
CrossoverSplitterNBands::CreateFilters(BL_FLOAT sampleRate)
{
    for (int i = 0; i < mNumBands; i++)
    {
        //int numFilters = mNumBands - 1;
        int chainSize = mNumBands - i;
        if (i == 0)
            chainSize = mNumBands - 1 - i;
        
        for (int j = 0; j < chainSize; j++)
        {
            BL_FLOAT cutoffFreq;
            if (i < 2)
                cutoffFreq = mCutoffFreqs[j];
            else
            {
                int idx = j + i - 1;
                cutoffFreq = mCutoffFreqs[idx];
            }
            
            if ((i == 0) && (j == 0))
                // First chain, first filter
            {
                // Create LPF
                SO_LINKWITZ_RILEY_BPF *filter = new SO_LINKWITZ_RILEY_BPF();
                filter->calculate_coeffs(cutoffFreq, sampleRate);
                
                mFilterChains[i].push_back(filter);
            }
            else if ((i > 0) && (j == 0))
                // Other chains, first filter
            {
                // Create HPF
                SO_LINKWITZ_RILEY_HPF *filter = new SO_LINKWITZ_RILEY_HPF();
                filter->calculate_coeffs(cutoffFreq, sampleRate);
                
                mFilterChains[i].push_back(filter);
            }
            else if ((i > 0) && (j == 1))
                // Other chains, second filter
            {
                // Create LPF
                SO_LINKWITZ_RILEY_BPF *filter = new SO_LINKWITZ_RILEY_BPF();
                filter->calculate_coeffs(cutoffFreq, sampleRate);
                
                mFilterChains[i].push_back(filter);
            }
            else // (j > 0)
                // Next filters of the chain
            {
                // Create APF
                APF *filter = new APF();
                filter->calculate_coeffs(cutoffFreq, sampleRate);
                
                mFilterChains[i].push_back(filter);
            }
        }
    }
}

void
CrossoverSplitterNBands::SetFiltersValues()
{
    for (int i = 0; i < mNumBands; i++)
    {
        //int numFilters = mNumBands - 1;
        int chainSize = mNumBands - i;
        if (i == 0)
            chainSize = mNumBands - 1 - i;
        
        for (int j = 0; j < chainSize; j++)
        {
            BL_FLOAT cutoffFreq;
            if (i < 2)
                cutoffFreq = mCutoffFreqs[j];
            else
            {
                int idx = j + i - 1;
                cutoffFreq = mCutoffFreqs[idx];
            }
            
            if ((i == 0) && (j == 0))
                // First chain, first filter
            {
                // LPF
                SO_LINKWITZ_RILEY_BPF *filter = (SO_LINKWITZ_RILEY_BPF *)mFilterChains[i][j];
                filter->calculate_coeffs(cutoffFreq, mSampleRate);
            }
            else if ((i > 0) && (j == 0))
                // Other chains, first filter
            {
                // HPF
                SO_LINKWITZ_RILEY_HPF *filter = (SO_LINKWITZ_RILEY_HPF *)mFilterChains[i][j];
                filter->calculate_coeffs(cutoffFreq, mSampleRate);
            }
            else if ((i > 0) && (j == 1))
                // Other chains, second filter
            {
                // HPF
                SO_LINKWITZ_RILEY_BPF *filter = (SO_LINKWITZ_RILEY_BPF *)mFilterChains[i][j];
                filter->calculate_coeffs(cutoffFreq, mSampleRate);
            }
            else // (j > 0)
                // Next filters of the chain
            {
                // APF
                APF *filter = (APF *)mFilterChains[i][j];
                filter->calculate_coeffs(cutoffFreq, mSampleRate);
            }
        }
    }
}
