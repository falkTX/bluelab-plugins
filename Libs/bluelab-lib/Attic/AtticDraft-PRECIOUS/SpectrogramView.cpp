//
//  SpectrogramView.cpp
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#include <BLSpectrogram3.h>
#include <FftProcessObj14.h>

#include "SpectrogramView.h"

#define MIN_ZOOM 0.1
#define MAX_ZOOM 10.0

#define DEBUG_DISABLE_DATA_ZOOM 1

SpectrogramView::SpectrogramView(BLSpectrogram3 *spectrogram,
                                 FftProcessObj14 *fftObj,
                                 int maxNumCols)
{
    mSpectrogram = spectrogram;
    mFftObj = fftObj;
    
    mMaxNumCols = maxNumCols;
    
    mCenterPos = 0.5;
    
    mSamplePos = 0;
    mStartPos = 0;
    mEndPos = 0;
    
    mZoomFactor = 1.0;
    mDataZoomFactor = 1.0;
}

SpectrogramView::~SpectrogramView() {}

void
SpectrogramView::SetData(const vector<WDL_TypedBuf<double> > &channels)
{
    mChannels = channels;
    
    UpdateSpectrogramData();
}

void
SpectrogramView::SetPosition(double pos)
{
    mCenterPos = pos;
    
    if (mChannels.empty())
        return;
    
    long viewSize = mEndPos - mStartPos;
    
    long samplePos = mStartPos + pos*viewSize;
    
    SetSamplePosition(samplePos);
}

double
SpectrogramView::GetPosition()
{
    //double result = ((double)mSamplePos)/(mEndPos - mStartPos);
    //return result;
    
    return mCenterPos;
}

void
SpectrogramView::SetSamplePosition(long samplePos)
{
    mSamplePos = samplePos;
    
#if !DEBUG_DISABLE_DATA_ZOOM
    mDataZoomFactor = mZoomFactor;
    UpdateSpectrogramData();
#endif
}

void
SpectrogramView::SetZoomFactor(double zoomFactor)
{
    mZoomFactor = zoomFactor;
    
#if !DEBUG_DISABLE_DATA_ZOOM
    mDataZoomFactor = mZoomFactor;
    UpdateSpectrogramData();
#endif
    
    //fprintf(stderr, "zoom: %g\n", mZoomFactor);
    //fprintf(stderr, "zoom data: %g\n", mDataZoomFactor);
}

void
SpectrogramView::UpdateZoomFactor(double zoomChange)
{

    mZoomFactor *= zoomChange;
    
    if (mZoomFactor < MIN_ZOOM)
        mZoomFactor = MIN_ZOOM;
    
    if (mZoomFactor > MAX_ZOOM)
        mZoomFactor = MAX_ZOOM;
    
#if !DEBUG_DISABLE_DATA_ZOOM
    mDataZoomFactor = mZoomFactor;
    UpdateSpectrogramData();
#endif
    
    //fprintf(stderr, "zoom: %g\n", mZoomFactor);
    //fprintf(stderr, "zoom data: %g\n", mDataZoomFactor);
}

double
SpectrogramView::GetZoomFactor()
{
    return mZoomFactor;
}

void
SpectrogramView::SetDataZoomFactor(double zoom)
{
    mDataZoomFactor = zoom;
    
    //fprintf(stderr, "zoom: %g\n", mZoomFactor);
    //fprintf(stderr, "zoom data: %g\n", mDataZoomFactor);
}


double
SpectrogramView::GetDataZoomFactor()
{
    return mDataZoomFactor;
}

void
SpectrogramView::UpdateSpectrogramData()
{
    mSpectrogram->Reset();
    
    if (mChannels.empty())
        return;
        
    long channelSize = mChannels[0].GetSize();
    if (channelSize == 0)
        return;
    
#if 1 // OLD
    mStartPos = mSamplePos - 0.5*channelSize/mDataZoomFactor;
    if (mStartPos < 0)
        mStartPos = 0;
    
    mEndPos = mSamplePos + 0.5*channelSize/mDataZoomFactor;
    if (mEndPos > channelSize - 1)
        mEndPos = channelSize - 1;
#endif
    
#if 0 // WORKS FOR INIT
    mStartPos = 0;
    mEndPos = channelSize - 1;
#endif
    
    long numSamples = mEndPos - mStartPos;
    
    int overlapping = mFftObj->GetOverlapping();
    int bufferSize = mFftObj->GetBufferSize();
    
    // TODO: manage stereo
    // (for the moment, we manage only mono)
    
    vector<WDL_TypedBuf<double> > dummySc;
    vector<WDL_TypedBuf<double> > dummyOut;
    
    int numChunks = numSamples*overlapping/bufferSize;
    
    // Add only some parts of the data if it is very long
    //int step = numChunks/mMaxNumCols;
    //if (step == 0)
    //    step = 1;
    
    // TEST floating point step
    // For oversampling when the data is short
    double step = ((double)numChunks)/mMaxNumCols;
    
    //fprintf(stderr, "step: %g\n", step);
    
    int pos = mStartPos;
    while(pos < mEndPos)
    {
        WDL_TypedBuf<double> chunk;
        chunk.Add(&mChannels[0].Get()[pos], bufferSize);
            
        vector<WDL_TypedBuf<double> > in;
        in.resize(1);
        in[0] = chunk;
            
        mFftObj->Process(in, dummySc, NULL);
            
        pos += step*bufferSize;
    }
}
