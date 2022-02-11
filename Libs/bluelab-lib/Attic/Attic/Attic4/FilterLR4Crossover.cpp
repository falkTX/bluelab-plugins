//
//  FilterLR4Crossover.cpp
//  UST
//
//  Created by applematuer on 7/29/19.
//
//

#include "FilterLR4Crossover.h"

#ifndef M_PI
#define M_PI 3.141592653589793
#endif


class LRCrossoverFilter // LR4 crossover filter
{
private:
    struct filterCoefficents
    {
        BL_FLOAT a0, a1, a2, a3, a4;
    } lpco, hpco;
    
    BL_FLOAT b1co, b2co, b3co, b4co;
    
    struct
    {
        BL_FLOAT xm1 = 0.0f;
        BL_FLOAT xm2 = 0.0f;
        BL_FLOAT xm3 = 0.0f;
        BL_FLOAT xm4 = 0.0f;
        BL_FLOAT ym1 = 0.0f, ym2 = 0.0f, ym3 = 0.0f, ym4 = 0.0f;
    } hptemp, lptemp;
    
    BL_FLOAT coFreqRunningAv = 100.0f;
    
public:
    // Niko
    LRCrossoverFilter() {}
    LRCrossoverFilter(const LRCrossoverFilter &other)
    {
        lpco = other.lpco;
        hpco = other.hpco;
        
        b1co = other.b1co;
        b2co = other.b2co;
        b3co = other.b3co;
        b4co = other.b4co;
        
        hptemp = other.hptemp;
        lptemp = other.lptemp;
        
        coFreqRunningAv = other.coFreqRunningAv;
    }
    
    virtual ~LRCrossoverFilter() {}
    
    void setup(BL_FLOAT crossoverFrequency, BL_FLOAT sr);
    void processBlock(BL_FLOAT * in, BL_FLOAT * outHP, BL_FLOAT * outLP, int numSamples);
    
    void process(BL_FLOAT in, BL_FLOAT * outHP, BL_FLOAT * outLP);
};


void LRCrossoverFilter::setup(BL_FLOAT crossoverFrequency, BL_FLOAT sr)
{
    coFreqRunningAv = crossoverFrequency;
    
    BL_FLOAT cowc=2*M_PI*coFreqRunningAv;
    BL_FLOAT cowc2=cowc*cowc;
    BL_FLOAT cowc3=cowc2*cowc;
    BL_FLOAT cowc4=cowc2*cowc2;
    
    BL_FLOAT cok=cowc/tan(M_PI*coFreqRunningAv/sr);
    BL_FLOAT cok2=cok*cok;
    BL_FLOAT cok3=cok2*cok;
    BL_FLOAT cok4=cok2*cok2;
    BL_FLOAT sqrt2=sqrt(2);
    BL_FLOAT sq_tmp1 = sqrt2 * cowc3 * cok;
    BL_FLOAT sq_tmp2 = sqrt2 * cowc * cok3;
    BL_FLOAT a_tmp = 4*cowc2*cok2 + 2*sq_tmp1 + cok4 + 2*sq_tmp2+cowc4;
    
    b1co=(4*(cowc4+sq_tmp1-cok4-sq_tmp2))/a_tmp;
    
    
    b2co=(6*cowc4-8*cowc2*cok2+6*cok4)/a_tmp;
    
    
    b3co=(4*(cowc4-sq_tmp1+sq_tmp2-cok4))/a_tmp;
    
    
    b4co=(cok4-2*sq_tmp1+cowc4-2*sq_tmp2+4*cowc2*cok2)/a_tmp;
    
    
    
    //================================================
    // low-pass
    //================================================
    lpco.a0=cowc4/a_tmp;
    lpco.a1=4*cowc4/a_tmp;
    lpco.a2=6*cowc4/a_tmp;
    lpco.a3=lpco.a1;
    lpco.a4=lpco.a0;
    
    //=====================================================
    // high-pass
    //=====================================================
    hpco.a0=cok4/a_tmp;
    hpco.a1=-4*cok4/a_tmp;
    hpco.a2=6*cok4/a_tmp;
    hpco.a3=hpco.a1;
    hpco.a4=hpco.a0;
}

void
LRCrossoverFilter::processBlock(BL_FLOAT * in, BL_FLOAT * outHP, BL_FLOAT * outLP, int numSamples)
{
    BL_FLOAT tempx, tempy;
    for (int i = 0; i<numSamples; i++)
    {
        tempx=in[i];
        
        // High pass
        
        tempy = hpco.a0*tempx +
        hpco.a1*hptemp.xm1 +
        hpco.a2*hptemp.xm2 +
        hpco.a3*hptemp.xm3 +
        hpco.a4*hptemp.xm4 -
        b1co*hptemp.ym1 -
        b2co*hptemp.ym2 -
        b3co*hptemp.ym3 -
        b4co*hptemp.ym4;
        
        hptemp.xm4=hptemp.xm3;
        hptemp.xm3=hptemp.xm2;
        hptemp.xm2=hptemp.xm1;
        hptemp.xm1=tempx;
        hptemp.ym4=hptemp.ym3;
        hptemp.ym3=hptemp.ym2;
        hptemp.ym2=hptemp.ym1;
        hptemp.ym1=tempy;
        outHP[i]=tempy;
        
        assert(tempy<10000000);
        
        // Low pass
        
        tempy = lpco.a0*tempx +
        lpco.a1*lptemp.xm1 +
        lpco.a2*lptemp.xm2 +
        lpco.a3*lptemp.xm3 +
        lpco.a4*lptemp.xm4 -
        b1co*lptemp.ym1 -
        b2co*lptemp.ym2 -
        b3co*lptemp.ym3 -
        b4co*lptemp.ym4;
        
        lptemp.xm4=lptemp.xm3; // these are the same as hptemp and could be optimised away
        lptemp.xm3=lptemp.xm2;
        lptemp.xm2=lptemp.xm1;
        lptemp.xm1=tempx;
        lptemp.ym4=lptemp.ym3;
        lptemp.ym3=lptemp.ym2;
        lptemp.ym2=lptemp.ym1;
        lptemp.ym1=tempy;
        outLP[i] = tempy;
        
        assert(!isnan(outLP[i]));
    }
}

// Niko
void
LRCrossoverFilter::process(BL_FLOAT in, BL_FLOAT * outHP, BL_FLOAT * outLP)
{
    BL_FLOAT tempx, tempy;
    
    tempx = in;
    
    // High pass

    tempy = hpco.a0*tempx +
    hpco.a1*hptemp.xm1 +
    hpco.a2*hptemp.xm2 +
    hpco.a3*hptemp.xm3 +
    hpco.a4*hptemp.xm4 -
    b1co*hptemp.ym1 -
    b2co*hptemp.ym2 -
    b3co*hptemp.ym3 -
    b4co*hptemp.ym4;
        
    hptemp.xm4=hptemp.xm3;
    hptemp.xm3=hptemp.xm2;
    hptemp.xm2=hptemp.xm1;
    hptemp.xm1=tempx;
    hptemp.ym4=hptemp.ym3;
    hptemp.ym3=hptemp.ym2;
    hptemp.ym2=hptemp.ym1;
    hptemp.ym1=tempy;
    *outHP=tempy;
        
    assert(tempy<10000000);
        
    // Low pass
        
    tempy = lpco.a0*tempx +
    lpco.a1*lptemp.xm1 +
    lpco.a2*lptemp.xm2 +
    lpco.a3*lptemp.xm3 +
    lpco.a4*lptemp.xm4 -
    b1co*lptemp.ym1 -
    b2co*lptemp.ym2 -
    b3co*lptemp.ym3 -
    b4co*lptemp.ym4;
        
    lptemp.xm4=lptemp.xm3; // these are the same as hptemp and could be optimised away
    lptemp.xm3=lptemp.xm2;
    lptemp.xm2=lptemp.xm1;
    lptemp.xm1=tempx;
    lptemp.ym4=lptemp.ym3;
    lptemp.ym3=lptemp.ym2;
    lptemp.ym2=lptemp.ym1;
    lptemp.ym1=tempy;
    *outLP = tempy;
        
    assert(!isnan(*outLP));
}

//

FilterLR4Crossover::FilterLR4Crossover(BL_FLOAT cutoffFreq, BL_FLOAT sampleRate)
{
    mFilter = new LRCrossoverFilter();
    
    mCutoffFreq = 200.0;
    if ((cutoffFreq > 0.0) && (cutoffFreq < sampleRate/2.0))
        mCutoffFreq = cutoffFreq;
    
    mSampleRate = sampleRate;
    
    Init();
}

FilterLR4Crossover::FilterLR4Crossover()
{
    mFilter = new LRCrossoverFilter();
    mCutoffFreq = 200.0;
    mSampleRate = 44100.0;
    
    Init();
}

FilterLR4Crossover::FilterLR4Crossover(const FilterLR4Crossover &other)
{
    mFilter = new LRCrossoverFilter();
    
    mCutoffFreq = other.mCutoffFreq;
    mSampleRate = other.mSampleRate;
    
    Init();
}

FilterLR4Crossover::~FilterLR4Crossover()
{
    delete mFilter;
}

void
FilterLR4Crossover::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    Init();
}

BL_FLOAT
FilterLR4Crossover::GetSampleRate() const
{
    return mSampleRate;
}

BL_FLOAT
FilterLR4Crossover::GetCutoffFreq() const
{
    return mCutoffFreq;
}

void
FilterLR4Crossover::SetCutoffFreq(BL_FLOAT freq)
{
    if ((freq > 0.0) && (freq < mSampleRate/2.0))
        mCutoffFreq = freq;
    
    Init();
}

void
FilterLR4Crossover::Reset(BL_FLOAT cutoffFreq, BL_FLOAT sampleRate)
{
    if ((cutoffFreq > 0.0) && (cutoffFreq < sampleRate/2.0))
        mCutoffFreq = cutoffFreq;
    
    mSampleRate = sampleRate;
    
    Init();
}

void
FilterLR4Crossover::Process(BL_FLOAT inSample,
                            BL_FLOAT *lpOutSample,
                            BL_FLOAT *hpOutSample)
{
    mFilter->process(inSample, hpOutSample, lpOutSample);
}

void
FilterLR4Crossover::Process(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                            WDL_TypedBuf<BL_FLOAT> *lpOutSamples,
                            WDL_TypedBuf<BL_FLOAT> *hpOutSamples)
{
    lpOutSamples->Resize(inSamples.GetSize());
    hpOutSamples->Resize(inSamples.GetSize());
    
    mFilter->processBlock(inSamples.Get(),
                          hpOutSamples->Get(),
                          lpOutSamples->Get(),
                          inSamples.GetSize());
}

void
FilterLR4Crossover::Init()
{
    mFilter->setup(mCutoffFreq, mSampleRate);
}
