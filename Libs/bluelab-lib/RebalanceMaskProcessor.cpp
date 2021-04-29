#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include <algorithm>
using namespace std;

#include "RebalanceMaskProcessor.h"

// With 100, the slope is very steep
// (but the sound seems similar to 10).
#define MAX_GAMMA 10.0 //20.0 //100.0

RebalanceMaskProcessor::RebalanceMaskProcessor()
{
    // Parameters
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
        mSensitivities[i] = 1.0;

    // Masks contrasts, relative one to each other (soft/hard)
    mMasksContrast = 0.0;
}

RebalanceMaskProcessor::~RebalanceMaskProcessor() {}

void
RebalanceMaskProcessor::Process(const WDL_TypedBuf<BL_FLOAT> masks0[NUM_STEM_SOURCES],
                                WDL_TypedBuf<BL_FLOAT> *resultMask)
{
    //WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES];
    WDL_TypedBuf<BL_FLOAT> *masks = mTmpBuf0;
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
    {
        masks[i] = masks0[i];
    }
    
    NormalizeMasks(masks); // ??
    
    ApplySensitivity(masks);
    
    NormalizeMasks(masks);

    ApplyMasksContrast(masks);
    
    NormalizeMasks(masks); // ??

    // Do not normalize after Mix!
    // (otherwise we risk to have everything at 1 e.g when only vocal mix is not zero)
    ApplyMix(masks);
    
    resultMask->Resize(masks[0].GetSize());
    BLUtils::FillAllZero(resultMask);
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
    {
        BLUtils::AddValues(resultMask, masks[i]);
    }
}

void
RebalanceMaskProcessor::SetVocalMix(BL_FLOAT vocalMix)
{
    mMixes[0] = vocalMix;
}

void
RebalanceMaskProcessor::SetBassMix(BL_FLOAT bassMix)
{
    mMixes[1] = bassMix;
}

void
RebalanceMaskProcessor::SetDrumsMix(BL_FLOAT drumsMix)
{
    mMixes[2] = drumsMix;
}

void
RebalanceMaskProcessor::SetOtherMix(BL_FLOAT otherMix)
{
    mMixes[3] = otherMix;
}

void
RebalanceMaskProcessor::SetVocalSensitivity(BL_FLOAT vocalSensitivity)
{
    mSensitivities[0] = vocalSensitivity;
}

void
RebalanceMaskProcessor::SetBassSensitivity(BL_FLOAT bassSensitivity)
{
    mSensitivities[1] = bassSensitivity;
}

void
RebalanceMaskProcessor::SetDrumsSensitivity(BL_FLOAT drumsSensitivity)
{
    mSensitivities[2] = drumsSensitivity;
}

void
RebalanceMaskProcessor::SetOtherSensitivity(BL_FLOAT otherSensitivity)
{
    mSensitivities[3] = otherSensitivity;
}

void
RebalanceMaskProcessor::SetContrast(BL_FLOAT contrast)
{
    mMasksContrast = contrast;
}

void
RebalanceMaskProcessor::ApplySensitivitySoft(BL_FLOAT masks[NUM_STEM_SOURCES])
{
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
    {
        masks[i] *= mSensitivities[i];
    }
}

void
RebalanceMaskProcessor::
ApplySensitivity(WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES])
{
    // Do nothing for the moment
    return;
    
    for (int i = 0; i < masks[0].GetSize(); i++)
    {
        BL_FLOAT vals[NUM_STEM_SOURCES];
        for (int j = 0; j < NUM_STEM_SOURCES; j++)
            vals[j] = masks[j].Get()[i];
        
        ApplySensitivitySoft(vals);
        
        for (int j = 0; j < NUM_STEM_SOURCES; j++)
            masks[j].Get()[i] = vals[j];
    }
}

// NOTE: previously tried to apply the mix parameter on
// mask values converted in amp => not good
void
RebalanceMaskProcessor::ApplyMix(WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES])
{    
    for (int i = 0; i < NUM_STEM_SOURCES; i++)
    {            
        BLUtils::MultValues(&masks[i], mMixes[i]);
    }
}

void
RebalanceMaskProcessor::
NormalizeMasks(WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES])
{
    BL_FLOAT vals[NUM_STEM_SOURCES];
    BL_FLOAT tvals[NUM_STEM_SOURCES];
    
    for (int i = 0; i < masks[0].GetSize(); i++)
    {
        // Init
        for (int j = 0; j < NUM_STEM_SOURCES; j++)
            vals[j] = masks[j].Get()[i];
        
        BL_FLOAT sum = vals[0] + vals[1] + vals[2] + vals[3];
        
        if (sum > BL_EPS)
        {
            BL_FLOAT sumInv = 1.0/sum;
            for (int j = 0; j < NUM_STEM_SOURCES; j++)
            {
                //tvals[j] = vals[j]/sum;
                tvals[j] = vals[j]*sumInv;
                
                masks[j].Get()[i] = tvals[j];
            }
        }
    }
}

// GOOD!
// NOTE: With gamma=10, it is almost like keeping only the maximum value
//
void
RebalanceMaskProcessor::
ApplyMasksContrast(WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES])
{
    // For the moment, do nothing (pow() takes much resource)
    // Will need to copimize better, e.g with sigmoid
    return;
    
    vector<MaskContrastStruct> &mc = mTmpBuf1;
    mc.resize(NUM_STEM_SOURCES);
    
    BL_FLOAT gamma = 1.0 + mMasksContrast*(MAX_GAMMA - 1.0);
    
    for (int i = 0; i < masks[0].GetSize(); i++)
    {
        // Fill the structure
        for (int k = 0; k < NUM_STEM_SOURCES; k++)
        {
            mc[k].mMaskId = k;
            mc[k].mValue = masks[k].Get()[i];
        }
        
        // Sort
        sort(mc.begin(), mc.end(), MaskContrastStruct::ValueSmaller);
        
        // Normalize
        BL_FLOAT minValue = mc[0].mValue;
        BL_FLOAT maxValue = mc[3].mValue;
        
        if (std::fabs(maxValue - minValue) < BL_EPS)
            continue;
        
        for (int k = 0; k < NUM_STEM_SOURCES; k++)
        {
            mc[k].mValue = (mc[k].mValue - minValue)/(maxValue - minValue);
        }
        
        // Apply gamma
        // See: https://www.researchgate.net/figure/Gamma-curves-where-X-represents-the-normalized-pixel-intensity_fig1_280851965
        for (int k = 0; k < NUM_STEM_SOURCES; k++)
        {
            mc[k].mValue = std::pow(mc[k].mValue, gamma);
        }
        
        // Un-normalize
        for (int k = 0; k < NUM_STEM_SOURCES; k++)
        {
            mc[k].mValue *= maxValue;
        }
        
        // Result
        for (int k = 0; k < NUM_STEM_SOURCES; k++)
        {
            masks[mc[k].mMaskId].Get()[i] = mc[k].mValue;
        }
    }
}
