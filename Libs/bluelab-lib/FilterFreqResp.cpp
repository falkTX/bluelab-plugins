//
//  FilterFreqResp.cpp
//  UST
//
//  Created by applematuer on 7/29/19.
//
//

#include <FftProcessObj16.h>
#include <FilterLR4Crossover.h>
#include <BLUtils.h>

#include "FilterFreqResp.h"


FilterFreqResp::FilterFreqResp()
{
    FftProcessObj16::Init();
}

FilterFreqResp::~FilterFreqResp() {}

void
FilterFreqResp::GetFreqResp(WDL_TypedBuf<BL_FLOAT> *freqRespLo,
                            WDL_TypedBuf<BL_FLOAT> *freqRespHi,
                            int numSamples,
                            FilterLR4Crossover *filter)
{
    WDL_TypedBuf<BL_FLOAT> impulse;
    impulse.Resize(numSamples*2);
    
    GenImpulse(&impulse);

    WDL_TypedBuf<BL_FLOAT> lpOutSamples;
    WDL_TypedBuf<BL_FLOAT> hpOutSamples;
    filter->Process(impulse, &lpOutSamples, &hpOutSamples);
    
    GetFreqResp(lpOutSamples, freqRespLo);
    GetFreqResp(hpOutSamples, freqRespHi);
}

void
FilterFreqResp::GenImpulse(WDL_TypedBuf<BL_FLOAT> *impulse)
{
    if (impulse->GetSize() == 0)
        return;
    
    BLUtils::FillAllZero(impulse);
    impulse->Get()[0] = 1.0;
}

void
FilterFreqResp::GetFreqResp(const WDL_TypedBuf<BL_FLOAT> &filteredImpulse,
                            WDL_TypedBuf<BL_FLOAT> *freqResp)
{
    WDL_TypedBuf<BL_FLOAT> phases;
    FftProcessObj16::SamplesToHalfMagnPhases(filteredImpulse,
                                             freqResp, &phases);
}

#if (BL_GUI_TYPE_FLOAT!=BL_TYPE_FLOAT)
void
FilterFreqResp::GetFreqResp(const WDL_TypedBuf<BL_FLOAT> &filteredImpulse,
                            WDL_TypedBuf<BL_GUI_FLOAT> *freqResp)
{
    WDL_TypedBuf<BL_FLOAT> freqResp0;
    FftProcessObj16::SamplesToHalfMagns(filteredImpulse, &freqResp0);
    
    freqResp->Resize(freqResp0.GetSize());
    for (int i = 0; i < freqResp0.GetSize(); i++)
    {
        freqResp->Get()[i] = freqResp0.Get()[i];
    }
}
#endif
