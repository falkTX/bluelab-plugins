//
//  Scale.cpp
//
//  Created by Pan on 08/04/18.
//
//

#include <cmath>
#include <math.h> // for exp10

#include <BLTypes.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <MelScale.h>
#include <FilterBank.h>

#include <FastMath.h>

#include "Scale.h"

//#define LOG_SCALE_FACTOR 0.25
#define LOG_SCALE2_FACTOR 3.5

// Center on 1000Hz
// NOTE: the value of 100 is a hack for the moment,
// with this, the center freq is between 1000 and 2000Hz
#define LOG_CENTER_FREQ 100.0

//#define LOW_ZOOM_GAMMA 0.95
#define LOW_ZOOM_GAMMA 0.8

#define LOG_EPS 1e-35

Scale::Scale()
{
    mMelScale = new MelScale();

    for (int i = 0; i < NUM_FILTER_BANKS; i++)
        mFilterBanks[i] = NULL;
}

Scale::~Scale()
{
    delete mMelScale;

    for (int i = 0; i < NUM_FILTER_BANKS; i++)
    {
        if (mFilterBanks[i] != NULL)
            delete mFilterBanks[i];
    }
}

//template <typename FLOAT_TYPE>
BL_FLOAT
Scale::ApplyScale(Type scaleType,
                  BL_FLOAT x, BL_FLOAT minValue, BL_FLOAT maxValue)
{
    if (scaleType == NORMALIZED)
    {
        x = ValueToNormalized(x, minValue, maxValue);
    }
    else if (scaleType == DB)
    {
        x = NormalizedToDB(x, minValue, maxValue);
    }
    else if (scaleType == LOG)
    {
        x = NormalizedToLog(x, minValue, maxValue);
    }
    else if (scaleType == LOG10)
    {
        x = NormalizedToLog10(x, minValue, maxValue);
    }
#if 0
    else if (scaleType == LOG_FACTOR)
    {
        x = NormalizedToLogFactor(x, minValue, maxValue);
    }
#endif
    else if (scaleType == LOG_FACTOR)
    {
        x = NormalizedToLogScale(x);
    }
    else if ((scaleType == MEL) || (scaleType == MEL_FILTER))
    {
        x = NormalizedToMel(x, minValue, maxValue);
    }
    else if ((scaleType == MEL_INV) || (scaleType == MEL_FILTER_INV))
    {
        x = NormalizedToMelInv(x, minValue, maxValue);
    }
    else if (scaleType == DB_INV)
    {
        x = NormalizedToDBInv(x, minValue, maxValue);
    }
    else if (scaleType == LOW_ZOOM)
    {
        x = NormalizedToLowZoom(x, minValue, maxValue);
    }
    else if (scaleType == LOG_NO_NORM)
    {
        x = ToLog(x);
    }
    else if (scaleType == LOG_NO_NORM_INV)
    {
        x = ToLogInv(x);
    }
    
    return x;
}
//template float Scale::ApplyScale(Type scaleType, float y, float mindB, float maxdB);
//template double Scale::ApplyScale(Type scaleType, double y, double mindB, double maxdB);

//template <typename FLOAT_TYPE>
BL_FLOAT
Scale::ApplyScaleInv(Type scaleType,
                     BL_FLOAT x, BL_FLOAT minValue, BL_FLOAT maxValue)
{
    if (scaleType == NORMALIZED)
    {
        x = ValueToNormalizedInv(x, minValue, maxValue);
    }
    else if (scaleType == DB)
    {
        x = NormalizedToDBInv(x, minValue, maxValue);
    }
    else if (scaleType == LOG)
    {
        x = NormalizedToLogInv(x, minValue, maxValue);
    }
    else if (scaleType == LOG10)
    {
        x = NormalizedToLog10Inv(x, minValue, maxValue);
    }
#if 0
    else if (scaleType == LOG_FACTOR)
    {
        x = NormalizedToLogFactorInv(x, minValue, maxValue);
    }
#endif
    else if (scaleType == LOG_FACTOR)
    {
        x = NormalizedToLogScaleInv(x);
    }
    else if ((scaleType == MEL) || (scaleType == MEL_FILTER))
    {
        x = NormalizedToMelInv(x, minValue, maxValue);
    }
    else if (scaleType == LOW_ZOOM)
    {
        x = NormalizedToLowZoomInv(x, minValue, maxValue);
    }
    else if (scaleType == LOG_NO_NORM)
    {
        x = ToLogInv(x);
    }
    
    return x;
}
//template float Scale::ApplyScaleInv(Type scaleType, float y, float mindB, float maxdB);
//template double Scale::ApplyScaleInv(Type scaleType, double y, double mindB, double maxdB);

void
Scale::ApplyScaleForEach(Type scaleType,
                         WDL_TypedBuf<BL_FLOAT> *values,
                         BL_FLOAT minValue,
                         BL_FLOAT maxValue)
{
    if (scaleType == NORMALIZED)
    {
        ValueToNormalizedForEach(values, minValue, maxValue);
    }
    else if (scaleType == DB)
    {
        NormalizedToDBForEach(values, minValue, maxValue);
    }
    else if (scaleType == LOG)
    {
        NormalizedToLogForEach(values, minValue, maxValue);
    }
    else if (scaleType == LOG10)
    {
        NormalizedToLog10ForEach(values, minValue, maxValue);
    }
#if 0
    else if (scaleType == LOG_FACTOR)
    {
        NormalizedToLogFactorForEach(values, minValue, maxValue);
    }
#endif
    else if (scaleType == LOG_FACTOR)
    {
        NormalizedToLogScaleForEach(values);
    }
    else if ((scaleType == MEL) || (scaleType == MEL_FILTER))
    {
        NormalizedToMelForEach(values, minValue, maxValue);
    }
    else if ((scaleType == MEL_INV) || (scaleType == MEL_FILTER_INV))
    {
        NormalizedToMelInvForEach(values, minValue, maxValue);
    }
    else if (scaleType == DB_INV)
    {
        NormalizedToDBInvForEach(values, minValue, maxValue);
    }
    else if (scaleType == LOW_ZOOM)
    {
        NormalizedToLowZoomForEach(values, minValue, maxValue);
    }
    else if (scaleType == LOG_NO_NORM)
    {
        ToLogForEach(values);
    }
    else if (scaleType == LOG_NO_NORM_INV)
    {
        ToLogInvForEach(values);
    }
}
    
void
Scale::ApplyScaleInvForEach(Type scaleType,
                            WDL_TypedBuf<BL_FLOAT> *values,
                            BL_FLOAT minValue,
                            BL_FLOAT maxValue)
{
    if (scaleType == NORMALIZED)
    {
        ValueToNormalizedInvForEach(values, minValue, maxValue);
    }
    else if (scaleType == DB)
    {
        NormalizedToDBInvForEach(values, minValue, maxValue);
    }
    else if (scaleType == LOG)
    {
        NormalizedToLogInvForEach(values, minValue, maxValue);
    }
    else if (scaleType == LOG10)
    {
        NormalizedToLog10InvForEach(values, minValue, maxValue);
    }
#if 0
    else if (scaleType == LOG_FACTOR)
    {
        NormalizedToLogFactorInvForEach(values, minValue, maxValue);
    }
#endif
    else if (scaleType == LOG_FACTOR)
    {
        NormalizedToLogScaleInvForEach(values);
    }
    else if ((scaleType == MEL) || (scaleType == MEL_FILTER))
    {
        NormalizedToMelInvForEach(values, minValue, maxValue);
    }
    else if (scaleType == LOW_ZOOM)
    {
        NormalizedToLowZoomInvForEach(values, minValue, maxValue);
    }
    else if (scaleType == LOG_NO_NORM)
    {
        ToLogInvForEach(values);
    }
}

//template <typename FLOAT_TYPE>
void
Scale::ApplyScale(Type scaleType,
                  WDL_TypedBuf<BL_FLOAT> *values,
                  BL_FLOAT minValue, BL_FLOAT maxValue)
{
    if (scaleType == LOG_FACTOR)
    {
        DataToLogScale(values);
    }
    else if (scaleType == MEL)
    {
        DataToMel(values, minValue, maxValue);
    }
    else if (scaleType == MEL_FILTER)
    {
        DataToMelFilter(values, minValue, maxValue);
    }
    else if (scaleType == MEL_FILTER_INV)
    {
        DataToMelFilterInv(values, minValue, maxValue);
    }
}
//// TMP HACK
////template void Scale::ApplyScale(Type scaleType, WDL_TypedBuf<float> *values,
////                                float minValue, float maxValue);
////template void Scale::ApplyScale(Type scaleType, WDL_TypedBuf<double> *values,
////                                double minValue, double maxValue);
//template void Scale::ApplyScale(Type scaleType, WDL_TypedBuf<BL_FLOAT> *values,
//                                BL_FLOAT minValue, BL_FLOAT maxValue);

void
Scale::ApplyScaleFilterBank(FilterBankType fbType,
                            WDL_TypedBuf<BL_FLOAT> *result,
                            const WDL_TypedBuf<BL_FLOAT> &magns,
                            BL_FLOAT sampleRate, int numFilters)
{
    if (fbType == FILTER_BANK_LINEAR)
    {
        // Try to not apply filter bank
        // Because even in linear, it modifies the data a little

        if (magns.GetSize() == numFilters)
            // Size is the same, nothing to do, just copy
        {
            *result = magns;
            
            return;
        }

        // Otherwise, will need filter bank to resize the data
    }
    
    if (mFilterBanks[(int)fbType] == NULL)
    {
        Type type = FilterBankTypeToType(fbType);
        mFilterBanks[(int)fbType] = new FilterBank(type);
    }
    
    mFilterBanks[(int)fbType]->HzToTarget(result, magns, sampleRate, numFilters);    
}

void
Scale::ApplyScaleFilterBankInv(FilterBankType fbType,
                               WDL_TypedBuf<BL_FLOAT> *result,
                               const WDL_TypedBuf<BL_FLOAT> &magns,
                               BL_FLOAT sampleRate, int numFilters)
{
    if (fbType == FILTER_BANK_LINEAR)
    {
        // Try to not apply filter bank
        // Because even in linear, it modifies the data a little

        if (magns.GetSize() == numFilters)
            // Size is the same, nothing to do, just copy
        {
            *result = magns;
            
            return;
        }

        // Otherwise, will need filter bank to resize the data
    }
    
    if (mFilterBanks[(int)fbType] == NULL)
    {
        Type type = FilterBankTypeToType(fbType);
        
        mFilterBanks[(int)fbType] = new FilterBank(type);
    }
    
    mFilterBanks[(int)fbType]->TargetToHz(result, magns, sampleRate, numFilters);
}

Scale::FilterBankType
Scale::TypeToFilterBankType(Type type)
{
    switch (type)
    {
        case LINEAR:
            return FILTER_BANK_LINEAR;
            break;

        case LOG:
            return FILTER_BANK_LOG;
            break;
            
        case LOG10:
            return FILTER_BANK_LOG10;
            break;

        case LOG_FACTOR:
            return FILTER_BANK_LOG_FACTOR;
            break;

        case MEL:
        case MEL_FILTER:
            return FILTER_BANK_MEL;
            break;

        case LOW_ZOOM:
            return FILTER_BANK_LOW_ZOOM;
            break;
            
        default:
            return (FilterBankType)-1;
    }
}

Scale::Type
Scale::FilterBankTypeToType(FilterBankType fbType)
{
    switch (fbType)
    {
        case FILTER_BANK_LINEAR:
            return LINEAR;
            break;

        case FILTER_BANK_LOG:
            return LOG;
            break;
            
        case FILTER_BANK_LOG10:
            return LOG10;
            break;

        case FILTER_BANK_LOG_FACTOR:
            return LOG_FACTOR;
            break;

        case FILTER_BANK_MEL:
            return MEL;
            break;

        case FILTER_BANK_LOW_ZOOM:
            return LOW_ZOOM;
            break;
            
        default:
            return (Type)-1;
    }
}

BL_FLOAT
Scale::ValueToNormalized(BL_FLOAT y,
                         BL_FLOAT minValue,
                         BL_FLOAT maxValue)
{
    y = (y - minValue)/(maxValue - minValue);

    return y;
}

BL_FLOAT
Scale::ValueToNormalizedInv(BL_FLOAT y,
                            BL_FLOAT minValue,
                            BL_FLOAT maxValue)
{
    y = y*(maxValue - minValue) + minValue;

    return y;
}

//template <typename FLOAT_TYPE>
BL_FLOAT
Scale::NormalizedToDB(BL_FLOAT x, BL_FLOAT mindB, BL_FLOAT maxdB)
{
    if (std::fabs(x) < BL_EPS)
        x = mindB;
    else
        x = BLUtils::AmpToDB(x);
    
    x = (x - mindB)/(maxdB - mindB);
    
    // Avoid negative values, for very low x dB
    if (x < 0.0)
        x = 0.0;
    
    return x;
}
//template float Scale::NormalizedToDB(float y, float mindB, float maxdB);
//template double Scale::NormalizedToDB(double y, double mindB, double maxdB);

//template <typename FLOAT_TYPE>
BL_FLOAT
Scale::NormalizedToDBInv(BL_FLOAT x, BL_FLOAT mindB, BL_FLOAT maxdB)
{
    x = mindB + x*(maxdB - mindB);
    
    x = BLUtils::DBToAmp(x);
 
    //BL_FLOAT minAmp = BLUtils::DBToAmp(mindB);
    //BL_FLOAT maxAmp = BLUtils::DBToAmp(maxdB);
    //x = (x - minAmp)/(maxAmp - minAmp);
    
    // Avoid negative values, for very low x dB
    if (x < 0.0)
        x = 0.0;
    
    return x;
}
//template float Scale::NormalizedToDBInv(float y, float mindB, float maxdB);
//template double Scale::NormalizedToDBInv(double y, double mindB, double maxdB);

//template <typename FLOAT_TYPE>
BL_FLOAT
Scale::NormalizedToLog10(BL_FLOAT x, BL_FLOAT minValue, BL_FLOAT maxValue)
{
    x = x*(maxValue - minValue) + minValue;
    
    //x = std::log10(1.0 + x);
    x = FastMath::log10(1.0 + x);
    
    //BL_FLOAT lMin = std::log10(1.0 + minValue);
    //BL_FLOAT lMax = std::log10(1.0 + maxValue);

    BL_FLOAT lMin = FastMath::log10(1.0 + minValue);
    BL_FLOAT lMax = FastMath::log10(1.0 + maxValue);
    
    x = (x - lMin)/(lMax - lMin);
    
    return x;
}
//template float Scale::NormalizedToLog(float x, float mindB, float maxdB);
//template double Scale::NormalizedToLog(double x, double mindB, double maxdB);

//template <typename FLOAT_TYPE>
BL_FLOAT
Scale::NormalizedToLog10Inv(BL_FLOAT x, BL_FLOAT minValue, BL_FLOAT maxValue)
{
    //BL_FLOAT lMin = std::log10(1.0 + minValue);
    //BL_FLOAT lMax = std::log10(1.0 + maxValue);
    BL_FLOAT lMin = FastMath::log10(1.0 + minValue);
    BL_FLOAT lMax = FastMath::log10(1.0 + maxValue);
    
    x = x*(lMax - lMin) + lMin;
    
    //x = std::pow((BL_FLOAT)10.0, x) - 1.0;
    x = FastMath::pow((BL_FLOAT)10.0, x) - 1.0;
    
    x = (x - minValue)/(maxValue - minValue);
    
    return x;
}
//template float Scale::NormalizedToLogInv(float x, float mindB, float maxdB);
//template double Scale::NormalizedToLogInv(double x, double mindB, double maxdB);

BL_FLOAT
Scale::NormalizedToLog(BL_FLOAT x, BL_FLOAT minValue, BL_FLOAT maxValue)
{
    //BL_FLOAT a = 0.5*maxValue/log(LOG_CENTER_FREQ);
    //BL_FLOAT a = (exp(0.5) - 1.0)/LOG_CENTER_FREQ;
    BL_FLOAT a = (FastMath::pow(10.0, 0.5) - 1.0)/LOG_CENTER_FREQ;
 
    BL_FLOAT lMin = FastMath::log10(1.0 + minValue*a);
    BL_FLOAT lMax = FastMath::log10(1.0 + maxValue*a);
    
    x = x*(maxValue - minValue) + minValue;
    
    x = FastMath::log10(1.0 + x*a);
    
    x = (x - lMin)/(lMax - lMin);
    
    return x;
}

BL_FLOAT
Scale::NormalizedToLogInv(BL_FLOAT x, BL_FLOAT minValue, BL_FLOAT maxValue)
{
    //BL_FLOAT a = 0.5*maxValue/log(LOG_CENTER_FREQ);
    BL_FLOAT a = (FastMath::pow(10.0, 0.5) - 1.0)/LOG_CENTER_FREQ;
    
    BL_FLOAT lMin = FastMath::log10(1.0 + minValue*a);
    BL_FLOAT lMax = FastMath::log10(1.0 + maxValue*a);
    
    x = x*(lMax - lMin) + lMin;
    
    x = (FastMath::pow((BL_FLOAT)10.0, x) - 1.0)/a;
    
    x = (x - minValue)/(maxValue - minValue);
    
    return x;
}


#if 0 // Legacy test
//template <typename FLOAT_TYPE>
BL_FLOAT
Scale::NormalizedToLogCoeff(BL_FLOAT x, BL_FLOAT minValue, BL_FLOAT maxValue)
{
    // This trick may be improved!
    x *= LOG_SCALE_FACTOR;
    minValue *= LOG_SCALE_FACTOR;
    maxValue *= LOG_SCALE_FACTOR;
    
    x = NormalizedToLog(x, minValue, maxValue);
    
    return x;
}
//template float Scale::NormalizedToLogCoeff(float x, float minValue, float maxValue);
//template double Scale::NormalizedToLogCoeff(double x, double minValue, double maxValue);
#endif

// Not well tested

//template <typename FLOAT_TYPE>
BL_FLOAT
Scale::NormalizedToLogScale(BL_FLOAT value)
{
    //BL_FLOAT t0 = std::log((BL_FLOAT)1.0 + value*(std::exp(LOG_SCALE2_FACTOR) - 1.0))/LOG_SCALE2_FACTOR;

    BL_FLOAT t0 = FastMath::log((BL_FLOAT)1.0 + value*(FastMath::exp(LOG_SCALE2_FACTOR) - 1.0))/LOG_SCALE2_FACTOR;
    
    return t0;
}
//template float Scale::NormalizedToLogScale(float value);
//template double Scale::NormalizedToLogScale(double value);

//template <typename FLOAT_TYPE>
BL_FLOAT
Scale::NormalizedToLogScaleInv(BL_FLOAT value)
{
    //BL_FLOAT t0 = (std::exp(value) - 1.0)/
    (((std::exp(LOG_SCALE2_FACTOR) - 1.0))/LOG_SCALE2_FACTOR);
    BL_FLOAT t0 = (FastMath::exp(value*LOG_SCALE2_FACTOR) - 1.0)/
        FastMath::exp(LOG_SCALE2_FACTOR - 1.0);
    
    return t0;
}
//template float Scale::NormalizedToLogScaleInv(float value);
//template double Scale::NormalizedToLogScaleInv(double value);

BL_FLOAT
Scale::NormalizedToLowZoom(BL_FLOAT x, BL_FLOAT minValue, BL_FLOAT maxValue)
{
    // 2 times mel
    BL_FLOAT result = NormalizedToMel(x, minValue, maxValue);
    //result = NormalizedToMel(result, minValue, maxValue); // double mel
    result = BLUtilsMath::ApplyGamma(result, (BL_FLOAT)LOW_ZOOM_GAMMA); // mel + gamma

    return result;
}
    
BL_FLOAT
Scale::NormalizedToLowZoomInv(BL_FLOAT x, BL_FLOAT minValue, BL_FLOAT maxValue)
{
    // 2 times mel inv
    //BL_FLOAT result = NormalizedToMelInv(x, minValue, maxValue);
    //result = NormalizedToMelInv(result, minValue, maxValue); // double mel

    BL_FLOAT result =
        BLUtilsMath::ApplyGamma(x, (BL_FLOAT)(1.0 - LOW_ZOOM_GAMMA)); // mel + gamma
    result = NormalizedToMelInv(result, minValue, maxValue);
        
    return result;
}

//template <typename FLOAT_TYPE>
void
Scale::DataToLogScale(WDL_TypedBuf<BL_FLOAT> *values)
{
    WDL_TypedBuf<BL_FLOAT> &origValues = mTmpBuf0;
    origValues = *values;
    
    int valuesSize = values->GetSize();
    BL_FLOAT *valuesData = values->Get();
    BL_FLOAT *origValuesData = origValues.Get();
    
    for (int i = 0; i < valuesSize; i++)
    {
        BL_FLOAT t0 = ((BL_FLOAT)i)/valuesSize;
        
        // "Inverse" process for data
        t0 *= LOG_SCALE2_FACTOR;
        //BL_FLOAT t = (std::exp(t0) - 1.0)/(std::exp(LOG_SCALE2_FACTOR) - 1.0);
        BL_FLOAT t =
            (FastMath::exp(t0) - 1.0)/(FastMath::exp(LOG_SCALE2_FACTOR) - 1.0);
        
        int dstIdx = (int)(t*valuesSize);
        
        if (dstIdx < 0)
            // Should not happen
            dstIdx = 0;
        
        if (dstIdx > valuesSize - 1)
            // We never know...
            dstIdx = valuesSize - 1;
        
        BL_FLOAT dstVal = origValuesData[dstIdx];
        valuesData[i] = dstVal;
    }
}
//template void Scale::DataToLogScale(WDL_TypedBuf<float> *values);
//template void Scale::DataToLogScale(WDL_TypedBuf<double> *values);

//template <typename FLOAT_TYPE>
BL_FLOAT
Scale::NormalizedToMel(BL_FLOAT x,
                       BL_FLOAT minFreq,
                       BL_FLOAT maxFreq)
{
    x = x*(maxFreq - minFreq) + minFreq;
    
    x = MelScale::HzToMel(x);
    
    BL_FLOAT lMin = MelScale::HzToMel(minFreq);
    BL_FLOAT lMax = MelScale::HzToMel(maxFreq);
    
    x = (x - lMin)/(lMax - lMin);
    
    return x;
}
//template float Scale::NormalizedToMel(float value,
//                                      float minFreq, float maxFreq);
//template double Scale::NormalizedToMel(double value,
//                                       double minFreq, double maxFreq);

//template <typename FLOAT_TYPE>
BL_FLOAT
Scale::NormalizedToMelInv(BL_FLOAT x,
                          BL_FLOAT minFreq,
                          BL_FLOAT maxFreq)
{
    BL_FLOAT minMel = MelScale::HzToMel(minFreq);
    BL_FLOAT maxMel = MelScale::HzToMel(maxFreq);
    
    x = x*(maxMel - minMel) + minMel;
    
    x = MelScale::MelToHz(x);
    
    x = (x - minFreq)/(maxFreq - minFreq);
    
    return x;
}
//template float Scale::NormalizedToMelInv(float value,
//                                         float minFreq, float maxFreq);
//template double Scale::NormalizedToMelInv(double value,
//                                          double minFreq, double maxFreq);

BL_FLOAT
Scale::ToLog(BL_FLOAT x)
{
    x = log(x + LOG_EPS);

    return x;
}
    
BL_FLOAT
Scale::ToLogInv(BL_FLOAT x)
{
    x = exp(x);

    return x;
}

//template <typename FLOAT_TYPE>
void
Scale::DataToMel(WDL_TypedBuf<BL_FLOAT> *values,
                 BL_FLOAT minFreq, BL_FLOAT maxFreq)
{
    WDL_TypedBuf<BL_FLOAT> &origValues = mTmpBuf1;
    origValues = *values;
    
    int valuesSize = values->GetSize();
    BL_FLOAT *valuesData = values->Get();
    BL_FLOAT *origValuesData = origValues.Get();
    
    BL_FLOAT minMel = MelScale::HzToMel(minFreq);
    BL_FLOAT maxMel = MelScale::HzToMel(maxFreq);

    BL_FLOAT t0 = 0.0;
    BL_FLOAT t0incr = 1.0/valuesSize;

    BL_FLOAT freqCoeff = 1.0/(maxFreq - minFreq);
    for (int i = 0; i < valuesSize; i++)
    {
        //BL_FLOAT t0 = ((BL_FLOAT)i)/valuesSize;

        // "Inverse" process for data
        BL_FLOAT mel = minMel + t0*(maxMel - minMel);
        BL_FLOAT freq = MelScale::MelToHz(mel);
        //BL_FLOAT t = (freq - minFreq)/(maxFreq - minFreq);
        BL_FLOAT t = (freq - minFreq)*freqCoeff;

        int dstIdx = (int)(t*valuesSize);
        
        if (dstIdx < 0)
            // Should not happen
            dstIdx = 0;
        
        if (dstIdx > valuesSize - 1)
            // We never know...
            dstIdx = valuesSize - 1;
        
        BL_FLOAT dstVal = origValuesData[dstIdx];
        valuesData[i] = dstVal;

        t0 += t0incr;
    }
}
//template void Scale::DataToMel(WDL_TypedBuf<float> *values,
//                               float minFreq, float maxFreq);
//template void Scale::DataToMel(WDL_TypedBuf<double> *values,
//                               double minFreq, double maxFreq);

//template <typename FLOAT_TYPE>
void
Scale::DataToMelFilter(WDL_TypedBuf<BL_FLOAT> *values,
                       BL_FLOAT minFreq, BL_FLOAT maxFreq)
{
    int numFilters = values->GetSize();
    WDL_TypedBuf<BL_FLOAT> &result = mTmpBuf2;
    mMelScale->HzToMelFilter(&result, *values, (BL_FLOAT)(maxFreq*2.0), numFilters);
    
    *values = result;
}
//// TMP HACK
////template void Scale::DataToMelFilter(WDL_TypedBuf<float> *values,
////                                     float minFreq, float maxFreq);
////template void Scale::DataToMelFilter(WDL_TypedBuf<double> *values,
////                                     double minFreq, double maxFreq);
//template void Scale::DataToMelFilter(WDL_TypedBuf<BL_FLOAT> *values,
//                                     BL_FLOAT minFreq, BL_FLOAT maxFreq);

//template <typename FLOAT_TYPE>
void
Scale::DataToMelFilterInv(WDL_TypedBuf<BL_FLOAT> *values,
                          BL_FLOAT minFreq, BL_FLOAT maxFreq)
{
    int numFilters = values->GetSize();
    WDL_TypedBuf<BL_FLOAT> &result = mTmpBuf3;
    mMelScale->MelToHzFilter(&result, *values, (BL_FLOAT)(maxFreq*2.0), numFilters);
    
    *values = result;
}
//// TMP HACK
////template void Scale::DataToMelFilterInv(WDL_TypedBuf<float> *values,
////                                        float minFreq, float maxFreq);
////template void Scale::DataToMelFilterInv(WDL_TypedBuf<double> *values,
////                                        double minFreq, double maxFreq);
//template void Scale::DataToMelFilterInv(WDL_TypedBuf<BL_FLOAT> *values,
//                                        BL_FLOAT minFreq, BL_FLOAT maxFreq);

// OPTIMS: compute in loops, to optimize some points
//

void
Scale::ValueToNormalizedForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                BL_FLOAT minValue, BL_FLOAT maxValue)
{
    int numValues = values->GetSize();
    BL_FLOAT *valuesData = values->Get();

    BL_FLOAT coeffInv = 0.0;
    if (maxValue - minValue > BL_EPS)
        coeffInv = 1.0/(maxValue - minValue);
    
    for (int i = 0; i < numValues; i++)
    {
        BL_FLOAT x = valuesData[i];

        //x = (x - minValue)/(maxValue - minValue);
        x = (x - minValue)*coeffInv;
        
         valuesData[i] = x;
    }
}
    
void
Scale::ValueToNormalizedInvForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                   BL_FLOAT minValue, BL_FLOAT maxValue)
{
    int numValues = values->GetSize();
    BL_FLOAT *valuesData = values->Get();

    for (int i = 0; i < numValues; i++)
    {
        BL_FLOAT x = valuesData[i];

        x = x*(maxValue - minValue) + minValue;
        
        valuesData[i] = x;
    }
}

void
Scale::NormalizedToDBForEach(WDL_TypedBuf<BL_FLOAT> *values,
                             BL_FLOAT mindB,
                             BL_FLOAT maxdB)
{
    int numValues = values->GetSize();
    BL_FLOAT *valuesData = values->Get();

    BL_FLOAT coeffInv = 0.0;
    if (maxdB - mindB > BL_EPS)
        coeffInv = 1.0/(maxdB - mindB);
    
    for (int i = 0; i < numValues; i++)
    {
        BL_FLOAT x = valuesData[i];
        
        if (std::fabs(x) < BL_EPS)
            x = mindB;
        else
            x = BLUtils::AmpToDB(x);
        
        //x = (x - mindB)/(maxdB - mindB);
        x = (x - mindB)*coeffInv;
        
        // Avoid negative values, for very low x dB
        if (x < 0.0)
            x = 0.0;

        valuesData[i] = x;
    }
}

void
Scale::NormalizedToDBInvForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                BL_FLOAT mindB,
                                BL_FLOAT maxdB)
{
    int numValues = values->GetSize();
    BL_FLOAT *valuesData = values->Get();

    for (int i = 0; i < numValues; i++)
    {
        BL_FLOAT x = valuesData[i];
        
        x = mindB + x*(maxdB - mindB);
        
        x = BLUtils::DBToAmp(x);
        
        // Avoid negative values, for very low x dB
        if (x < 0.0)
            x = 0.0;

        valuesData[i] = x;
    }
}
    
void
Scale::NormalizedToLog10ForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                BL_FLOAT minValue,
                                BL_FLOAT maxValue)
{
    int numValues = values->GetSize();
    BL_FLOAT *valuesData = values->Get();

    //BL_FLOAT lMin = std::log10(1.0 + minValue);
    //BL_FLOAT lMax = std::log10(1.0 + maxValue);
    BL_FLOAT lMin = FastMath::log10(1.0 + minValue);
    BL_FLOAT lMax = FastMath::log10(1.0 + maxValue);
        
    BL_FLOAT coeffInv = 0.0;
    if (lMax - lMin > BL_EPS)
        coeffInv = 1.0/(lMax - lMin);
                        
    for (int i = 0; i < numValues; i++)
    {
        BL_FLOAT x = valuesData[i];
        
        x = x*(maxValue - minValue) + minValue;
    
        //x = std::log10(1.0 + x);
        x = FastMath::log10(1.0 + x);
    
        //x = (x - lMin)/(lMax - lMin);
        x = (x - lMin)*coeffInv;

        valuesData[i] = x;
    }
}
    
void
Scale::NormalizedToLog10InvForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                   BL_FLOAT minValue,
                                   BL_FLOAT maxValue)
{
    int numValues = values->GetSize();
    BL_FLOAT *valuesData = values->Get();

    //BL_FLOAT lMin = std::log10(1.0 + minValue);
    //BL_FLOAT lMax = std::log10(1.0 + maxValue);
    BL_FLOAT lMin = FastMath::log10(1.0 + minValue);
    BL_FLOAT lMax = FastMath::log10(1.0 + maxValue);
        
    BL_FLOAT coeffInv = 0.0;
    if (maxValue - minValue > BL_EPS)
        coeffInv = 1.0/(maxValue - minValue);
    
    for (int i = 0; i < numValues; i++)
    {
        BL_FLOAT x = valuesData[i];
        
        x = x*(lMax - lMin) + lMin;
    
        //x = std::pow((BL_FLOAT)10.0, x) - 1.0;
        x = FastMath::pow((BL_FLOAT)10.0, x) - 1.0;
    
        //x = (x - minValue)/(maxValue - minValue);
        x = (x - minValue)*coeffInv;

        valuesData[i] = x;
    }
}

void
Scale::NormalizedToLogForEach(WDL_TypedBuf<BL_FLOAT> *values,
                              BL_FLOAT minValue,
                              BL_FLOAT maxValue)
{
    //BL_FLOAT a = 0.5*maxValue/log(LOG_CENTER_FREQ);
    //BL_FLOAT a = (exp(0.5) - 1.0)/LOG_CENTER_FREQ;
    BL_FLOAT a = (FastMath::pow(10.0, 0.5) - 1.0)/LOG_CENTER_FREQ;
    
    int numValues = values->GetSize();
    BL_FLOAT *valuesData = values->Get();

    BL_FLOAT lMin = FastMath::log10(1.0 + minValue*a);
    BL_FLOAT lMax = FastMath::log10(1.0 + maxValue*a);
        
    BL_FLOAT coeffInv = 0.0;
    if (lMax - lMin > BL_EPS)
        coeffInv = 1.0/(lMax - lMin);
                        
    for (int i = 0; i < numValues; i++)
    {
        BL_FLOAT x = valuesData[i];
        
        x = x*(maxValue - minValue) + minValue;
    
        x = FastMath::log10(1.0 + x*a);
    
        x = (x - lMin)*coeffInv;

        valuesData[i] = x;
    }
}
    
void
Scale::NormalizedToLogInvForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                 BL_FLOAT minValue,
                                 BL_FLOAT maxValue)
{
    //BL_FLOAT a = 0.5*maxValue/log(LOG_CENTER_FREQ);
    //BL_FLOAT a = (exp(0.5) - 1.0)/LOG_CENTER_FREQ;
    BL_FLOAT a = (FastMath::pow(10.0, 0.5) - 1.0)/LOG_CENTER_FREQ;
    
    int numValues = values->GetSize();
    BL_FLOAT *valuesData = values->Get();

    BL_FLOAT lMin = FastMath::log10(1.0 + minValue*a);
    BL_FLOAT lMax = FastMath::log10(1.0 + maxValue*a);
        
    BL_FLOAT coeffInv = 0.0;
    if (maxValue - minValue > BL_EPS)
        coeffInv = 1.0/(maxValue - minValue);
    
    for (int i = 0; i < numValues; i++)
    {
        BL_FLOAT x = valuesData[i];
        
        x = x*(lMax - lMin) + lMin;
    
        x = (FastMath::pow((BL_FLOAT)10.0, x) - 1.0)/a;
    
        x = (x - minValue)*coeffInv;

        valuesData[i] = x;
    }
}

void
Scale::NormalizedToLogScaleForEach(WDL_TypedBuf<BL_FLOAT> *values)
{
    int numValues = values->GetSize();
    BL_FLOAT *valuesData = values->Get();

    BL_FLOAT coeffInv = 1.0/LOG_SCALE2_FACTOR;
    BL_FLOAT coeff2 = FastMath::exp(LOG_SCALE2_FACTOR) - 1.0;
    
    for (int i = 0; i < numValues; i++)
    {
        BL_FLOAT x = valuesData[i];
        
        //x = FastMath::log((BL_FLOAT)1.0 + x*(FastMath::exp(LOG_SCALE2_FACTOR) - 1.0))/LOG_SCALE2_FACTOR;
        //x = FastMath::log((BL_FLOAT)1.0 + x*(FastMath::exp(LOG_SCALE2_FACTOR) - 1.0))*coeffInv;
        x = FastMath::log((BL_FLOAT)1.0 + x*coeff2)*coeffInv;
        valuesData[i] = x;
    }
}
    
void
Scale::NormalizedToLogScaleInvForEach(WDL_TypedBuf<BL_FLOAT> *values)
{
    int numValues = values->GetSize();
    BL_FLOAT *valuesData = values->Get();

    //BL_FLOAT coeffInv = 1.0/LOG_SCALE2_FACTOR;
    //BL_FLOAT coeffInv = 1.0/(((FastMath::exp(LOG_SCALE2_FACTOR) - 1.0))/LOG_SCALE2_FACTOR);

    BL_FLOAT coeffInv = 1.0/FastMath::exp(LOG_SCALE2_FACTOR - 1.0);
    
    for (int i = 0; i < numValues; i++)
    {
        BL_FLOAT x = valuesData[i];
        
        //x = (FastMath::exp(x) - 1.0)/(((FastMath::exp(LOG_SCALE2_FACTOR) - 1.0))/LOG_SCALE2_FACTOR);
        //x = (FastMath::exp(x) - 1.0)/(((FastMath::exp(LOG_SCALE2_FACTOR) - 1.0))*coeffInv);
        //x = (FastMath::exp(x) - 1.0)*coeffInv;
        x = (FastMath::exp(x*LOG_SCALE2_FACTOR) - 1.0)*coeffInv;
        
        valuesData[i] = x;
    }
}
    
void
Scale::NormalizedToMelForEach(WDL_TypedBuf<BL_FLOAT> *values,
                              BL_FLOAT minFreq,
                              BL_FLOAT maxFreq)
{
    int numValues = values->GetSize();
    BL_FLOAT *valuesData = values->Get();

    BL_FLOAT lMin = MelScale::HzToMel(minFreq);
    BL_FLOAT lMax = MelScale::HzToMel(maxFreq);
        
    BL_FLOAT coeffInv = 0.0;
    if (lMax - lMin > BL_EPS)
        coeffInv = 1.0/(lMax - lMin);
    
    for (int i = 0; i < numValues; i++)
    {
        BL_FLOAT x = valuesData[i];
        
        x = x*(maxFreq - minFreq) + minFreq;
    
        x = MelScale::HzToMel(x);
    
        //x = (x - lMin)/(lMax - lMin);
        x = (x - lMin)*coeffInv;;

        valuesData[i] = x;
    }
}
    
void
Scale::NormalizedToMelInvForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                 BL_FLOAT minFreq,
                                 BL_FLOAT maxFreq)
{
    int numValues = values->GetSize();
    BL_FLOAT *valuesData = values->Get();

    BL_FLOAT minMel = MelScale::HzToMel(minFreq);
    BL_FLOAT maxMel = MelScale::HzToMel(maxFreq);
        
    BL_FLOAT coeffInv = 0.0;
    if (maxFreq - minFreq > BL_EPS)
        coeffInv = 1.0/(maxFreq - minFreq);
    
    for (int i = 0; i < numValues; i++)
    {
        BL_FLOAT x = valuesData[i];
    
        x = x*(maxMel - minMel) + minMel;
        
        x = MelScale::MelToHz(x);
    
        //x = (x - minFreq)/(maxFreq - minFreq);
        x = (x - minFreq)*coeffInv;
        
        valuesData[i] = x;
    }
}

void
Scale::NormalizedToLowZoomForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                  BL_FLOAT minFreq, BL_FLOAT maxFreq)
{
    // 2 times mel
    //NormalizedToMelForEach(values, minValue, maxValue);
    //NormalizedToMelForEach(values, minValue, maxValue);

    int numValues = values->GetSize();
    BL_FLOAT *valuesData = values->Get();

    BL_FLOAT lMin = MelScale::HzToMel(minFreq);
    //lMin = BLUtilsMath::ApplyGamma(lMin, LOW_ZOOM_GAMMA);
    
    BL_FLOAT lMax = MelScale::HzToMel(maxFreq);
    //lMax = BLUtilsMath::ApplyGamma(lMax, LOW_ZOOM_GAMMA);
        
    BL_FLOAT coeffInv = 0.0;
    if (lMax - lMin > BL_EPS)
        coeffInv = 1.0/(lMax - lMin);
    
    for (int i = 0; i < numValues; i++)
    {
        BL_FLOAT x = valuesData[i];
        
        x = x*(maxFreq - minFreq) + minFreq;
    
        x = MelScale::HzToMel(x);
        
        x = (x - lMin)*coeffInv;;

        x = BLUtilsMath::ApplyGamma(x, (BL_FLOAT)LOW_ZOOM_GAMMA);
        
        valuesData[i] = x;
    }
}
    
void
Scale::NormalizedToLowZoomInvForEach(WDL_TypedBuf<BL_FLOAT> *values,
                                     BL_FLOAT minValue, BL_FLOAT maxValue)
{
    // 2 times mel inv
    //NormalizedToMelInvForEach(values, minValue, maxValue);
    //NormalizedToMelInvForEach(values, minValue, maxValue);

    int numValues = values->GetSize();
    BL_FLOAT *valuesData = values->Get();

    BL_FLOAT minMel = MelScale::HzToMel(minValue);
    //minMel = BLUtilsMath::ApplyGamma(minMel, LOW_ZOOM_GAMMA);
    
    BL_FLOAT maxMel = MelScale::HzToMel(maxValue);
    //maxMel = BLUtilsMath::ApplyGamma(maxMel, LOW_ZOOM_GAMMA);
        
    BL_FLOAT coeffInv = 0.0;
    if (maxValue - minValue > BL_EPS)
        coeffInv = 1.0/(maxValue - minValue);
    
    for (int i = 0; i < numValues; i++)
    {
        BL_FLOAT x = valuesData[i];

        x = BLUtilsMath::ApplyGamma(x, (BL_FLOAT)(1.0 - LOW_ZOOM_GAMMA));
        
        x = x*(maxMel - minMel) + minMel;

        //x = BLUtilsMath::ApplyGamma(x, 1.0 - LOW_ZOOM_GAMMA);
        
        x = MelScale::MelToHz(x);
    
        x = (x - minValue)*coeffInv;
        
        valuesData[i] = x;
    }
}

void
Scale::ToLogForEach(WDL_TypedBuf<BL_FLOAT> *values)
{
    for (int i = 0; i < values->GetSize(); i++)
    {
        BL_FLOAT x = values->Get()[i];
        x = log(x + LOG_EPS);
        values->Get()[i] = x;
    }
}
    
void
Scale::ToLogInvForEach(WDL_TypedBuf<BL_FLOAT> *values)
{
    for (int i = 0; i < values->GetSize(); i++)
    {
        BL_FLOAT x = values->Get()[i];
        x = exp(x);
        values->Get()[i] = x;
    }
}
