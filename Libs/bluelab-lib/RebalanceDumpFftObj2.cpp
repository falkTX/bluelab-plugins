//
//  RebalanceDumpFftObj2.cpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/14/20.
//
//

#include <MelScale.h>
#include <Scale.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>

#include <StereoWidenProcess.h>

#include <Rebalance_defs.h>

#include "RebalanceDumpFftObj2.h"

#define STEREO_WIDTH_FACTOR 1000.0

RebalanceDumpFftObj2::RebalanceDumpFftObj2(int bufferSize,
                                           BL_FLOAT sampleRate,
                                           int numInputCols)
: MultichannelProcess()
{
    mNumInputCols = numInputCols;
    
    // Fill with zeros at the beginning
    mSpectroCols.resize(mNumInputCols);
    for (int i = 0; i < mSpectroCols.size(); i++)
    {
        BLUtils::ResizeFillZeros(&mSpectroCols[i], bufferSize/2);
    }

    // Fill with zeros at the beginning
    mStereoCols.resize(mNumInputCols);
    for (int i = 0; i < mStereoCols.size(); i++)
    {
        BLUtils::ResizeFillZeros(&mStereoCols[i], bufferSize/2);
    }
    
    mSampleRate = sampleRate;
    
    mMelScale = new MelScale();
    mScale = new Scale();
}

RebalanceDumpFftObj2::~RebalanceDumpFftObj2()
{
    delete mMelScale;
    delete mScale;
}

void
RebalanceDumpFftObj2::
ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
    ProcessSpectrogramData(ioFftSamples, scBuffer);
    ProcessStereoData(ioFftSamples, scBuffer);
}

bool
RebalanceDumpFftObj2::HasEnoughData()
{
    bool hasEnoughData = (mSpectroCols.size() >= mNumInputCols);
    
    return hasEnoughData;
}

void
RebalanceDumpFftObj2::GetSpectrogramData(WDL_TypedBuf<BL_FLOAT> cols[REBALANCE_NUM_SPECTRO_COLS])
{
    for (int i = 0; i < mNumInputCols; i++)
    {
        cols[i] = mSpectroCols[i];
    }
    
    for (int i = 0; i < mNumInputCols; i++)
    {
        mSpectroCols.pop_front();
    }
}

void
RebalanceDumpFftObj2::GetStereoData(WDL_TypedBuf<BL_FLOAT> cols[REBALANCE_NUM_SPECTRO_COLS])
{
    for (int i = 0; i < mNumInputCols; i++)
    {
        cols[i] = mStereoCols[i];
    }
    
    for (int i = 0; i < mNumInputCols; i++)
    {
        mStereoCols.pop_front();
    }
}

void
RebalanceDumpFftObj2::
ProcessSpectrogramData(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * >
                       *ioFftSamples,
                       const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> >
                       *scBuffer)
{
    // Stereo to mono
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > monoFftSamples;
    for (int i = 0; i < ioFftSamples->size(); i++)
    {
        monoFftSamples.push_back(*(*ioFftSamples)[i]);
    }
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> dataBuffer;
    BLUtils::StereoToMono(&dataBuffer, monoFftSamples);
    
    //
    BLUtils::TakeHalf(&dataBuffer);
    
    WDL_TypedBuf<BL_FLOAT> magnsMix;
    BLUtilsComp::ComplexToMagn(&magnsMix, dataBuffer);
    
    // Downsample and convert to mel
#if REBALANCE_MEL_METHOD_FILTER
    int numMelBins = REBALANCE_NUM_SPECTRO_FREQS;
    WDL_TypedBuf<BL_FLOAT> melMagnsFilters = magnsMix;
    mMelScale->HzToMelFilter(&melMagnsFilters, magnsMix, mSampleRate, numMelBins);
    magnsMix = melMagnsFilters;
#endif

#if REBALANCE_MEL_METHOD_SCALE
    int numMelBins = REBALANCE_NUM_SPECTRO_FREQS;
    // TODO: use tmp buffers
    WDL_TypedBuf<BL_FLOAT> melMagnsFilters;
    Scale::FilterBankType type = mScale->TypeToFilterBankType(Scale::MEL_FILTER);
    mScale->ApplyScaleFilterBank(type, &melMagnsFilters, magnsMix,
                                 mSampleRate, numMelBins);
    magnsMix = melMagnsFilters;
#endif
    
#if REBALANCE_MEL_METHOD_SIMPLE
    // Quick method
    BLUtils::ResizeLinear(magnsMix, REBALANCE_NUM_SPECTRO_FREQS);
    WDL_TypedBuf<BL_FLOAT> melMagnsFilters = magnsMix;
    MelScale::HzToMel(&melMagnsFilters, *ioMagns, mSampleRate);
    magnsMix = melMagnsFilters;
#endif
    
    mSpectroCols.push_back(magnsMix);
}

void
RebalanceDumpFftObj2::ProcessStereoData(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * >
                                        *ioFftSamples,
                                        const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> >
                                        *scBuffer)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples[2];
    fftSamples[0] = *(*ioFftSamples)[0];
    fftSamples[1] = *(*ioFftSamples)[1];
    
    WDL_TypedBuf<BL_FLOAT> magns[2];
    WDL_TypedBuf<BL_FLOAT> phases[2];
    
    for (int i = 0; i < 2; i++)
    {
        BLUtils::TakeHalf(&fftSamples[i]);
        BLUtilsComp::ComplexToMagnPhase(&magns[i], &phases[i], fftSamples[i]);
    }
    
    WDL_TypedBuf<BL_FLOAT> widthData;
    StereoWidenProcess::ComputeStereoWidth(magns, phases, &widthData);
    
    BLUtils::MultValues(&widthData, (BL_FLOAT)STEREO_WIDTH_FACTOR);
              
    // Downsample and convert to mel
#if REBALANCE_MEL_METHOD_FILTER
    int numMelBins = REBALANCE_NUM_SPECTRO_FREQS;
    WDL_TypedBuf<BL_FLOAT> melWidthFilters = widthData;
    mMelScale->HzToMelFilter(&melWidthFilters, widthData, mSampleRate, numMelBins);
    widthData = melWidthFilters;
#endif

#if REBALANCE_MEL_METHOD_SCALE
    int numMelBins = REBALANCE_NUM_SPECTRO_FREQS;
    // TODO: use tmp buffers
    WDL_TypedBuf<BL_FLOAT> melWidthFilters;
    Scale::FilterBankType type = mScale->TypeToFilterBankType(Scale::MEL_FILTER);
    mScale->ApplyScaleFilterBank(type, &melWidthFilters, widthData,
                                 mSampleRate, numMelBins);
    widthData = melWidthFilters;
#endif
    
#if REBALANCE_MEL_METHOD_SIMPLE
    // Quick method
    BLUtils::ResizeLinear(widthData, REBALANCE_NUM_SPECTRO_FREQS);
    WDL_TypedBuf<BL_FLOAT> melWidthFilters = widthData;
    MelScale::HzToMel(&melWidthFilters, widthData, mSampleRate);
    widthData = melWidthFilters;
#endif
    
    mStereoCols.push_back(widthData);
}
