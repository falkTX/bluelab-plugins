//
//  AWeighting.cpp
//  AutoGain
//
//  Created by Pan on 30/01/18.
//
//

#include <math.h>

#include <BLTypes.h>

#include "AWeighting.h"

#define DB_INF -70.0
#define DB_EPS 1e-15

void
AWeighting::ComputeAWeights(WDL_TypedBuf<BL_FLOAT> *result, int numBins, BL_FLOAT sampleRate)
{
    result->Resize(numBins);
    
    for (int i = 0; i < result->GetSize(); i++)
    {
        BL_FLOAT freq = i*(sampleRate/(numBins*2));
        
        BL_FLOAT a = ComputeA(freq);
        
        result->Get()[i] = a;
    }
}

BL_FLOAT
AWeighting::ComputeR(BL_FLOAT frequency)
{
  BL_FLOAT num = std::pow(12194, 2)*pow(frequency, 4);
    
  BL_FLOAT denom0 = std::pow(frequency, 2) + std::pow(20.6, 2);
    
  BL_FLOAT denom1_2_1 = std::pow(frequency, 2) + std::pow(107.7, 2);
  BL_FLOAT denom1_2_2 = std::pow(frequency, 2) + std::pow(737.9, 2);
    
    BL_FLOAT denom1 = std::sqrt(denom1_2_1*denom1_2_2);
    
    BL_FLOAT denom2 = std::pow(frequency, (BL_FLOAT)2) + std::pow(12194, 2);
    
    BL_FLOAT denom = denom0*denom1*denom2;
    
    BL_FLOAT r = num/denom;
    
    return r;
}

BL_FLOAT
AWeighting::ComputeA(BL_FLOAT frequency)
{
    BL_FLOAT r = ComputeR(frequency);
    
    // Be careful of log(0)
    if (r < DB_EPS)
        return DB_INF;
        
    BL_FLOAT a = 20.0*std::log(r)/std::log(10.0) + 2.0;
    
    return a;
}
