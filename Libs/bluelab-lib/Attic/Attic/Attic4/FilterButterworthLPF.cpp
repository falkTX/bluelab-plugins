//
//  FilterButterworthLPF.cpp
//  UST
//
//  Created by applematuer on 8/11/20.
//
//

#include <so_butterworth_lpf.h>

#include "FilterButterworthLPF.h"

FilterButterworthLPF::FilterButterworthLPF()
{
    mFilter = new SO_BUTTERWORTH_LPF();
}

FilterButterworthLPF::~FilterButterworthLPF()
{
    delete mFilter;
}

void
FilterButterworthLPF::Init(BL_FLOAT cutFreq, BL_FLOAT sampleRate)
{
    mFilter->calculate_coeffs(cutFreq, sampleRate);
}

BL_FLOAT
FilterButterworthLPF::Process(BL_FLOAT sample)
{
    sample = mFilter->filter(sample);
    
    return sample;
}
