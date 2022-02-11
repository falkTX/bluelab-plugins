//
//  PartialsToFreqCepstrum.cpp
//  BL-SASViewer
//
//  Created by applematuer on 2/25/19.
//
//

#include <algorithm>
using namespace std;

#include <PartialTWMEstimate2.h>

#include <FftProcessObj16.h>

#include <BLUtils.h>

#include "PartialsToFreqCepstrum.h"


#define EPS 1e-15
#define INF 1e15

#define MIN_AMP_DB -120.0

// 20Hz
#define MIN_FREQ_DETECT 50.0
#define MAX_FREQ_DETECT 2000.0  //22050.0

PartialsToFreqCepstrum::PartialsToFreqCepstrum(int bufferSize,
                                               BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
}

PartialsToFreqCepstrum::~PartialsToFreqCepstrum() {}

void
PartialsToFreqCepstrum::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
}

BL_FLOAT
PartialsToFreqCepstrum::ComputeFrequency(const vector<PartialTracker3::Partial> &partials)
{
    BL_FLOAT freq = 0.0;
    
    if (partials.empty())
        return 0.0;
    
    WDL_TypedBuf<BL_FLOAT> magns;
    PartialsToMagns(partials, &magns);
    
    WDL_TypedBuf<BL_FLOAT> cepstrum;
    FftProcessObj16::MagnsToCepstrum(magns, &cepstrum);
    
    // Avoid finiding the maximum peak under the min frequency
    int minIndex = mSampleRate/MAX_FREQ_DETECT;
    for (int i = 0; i < minIndex; i++)
    {
        if (i >= cepstrum.GetSize())
            break;
        
        cepstrum.Get()[i] = 0.0;
    }
    
    int maxIndex = mSampleRate/MIN_FREQ_DETECT;
    for (int i = maxIndex; i < cepstrum.GetSize(); i++)
        cepstrum.Get()[i] = 0.0;
    
    int index = BLUtils::FindMaxIndex(cepstrum);
    
    if (index > 0)
        freq = mSampleRate/index;
    
    return freq;
}

void
PartialsToFreqCepstrum::PartialsToMagns(const vector<PartialTracker3::Partial> &partials,
                                        WDL_TypedBuf<BL_FLOAT> *magns)
{
    magns->Resize(mBufferSize/2);
    BLUtils::FillAllZero(magns);
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &partial = partials[i];
        
        BL_FLOAT freq = partial.mFreq;
        
        BL_FLOAT bin = freq/hzPerBin;
        bin = bl_round(bin);
        
        if (bin >= magns->GetSize())
            continue;
        
        BL_FLOAT ampDB = partial.mAmpDB;
        BL_FLOAT amp = BLUtils::DBToAmp(ampDB);
        
        magns->Get()[(int)bin] = amp;
    }
}
