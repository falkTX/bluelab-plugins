//
//  SpectrogramView.cpp
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#include <BLSpectrogram3.h>
#include <FftProcessObj14.h>
#include <Utils.h>

#include "SpectrogramView.h"

#define MIN_ZOOM 1.0
#define MAX_ZOOM 20.0

// Without, we have no vertical artifact lines
// With, we can zoom more with real zoomed data displayed
#define USE_FLOAT_STEP 0

SpectrogramView::SpectrogramView(BLSpectrogram3 *spectrogram,
                                 FftProcessObj14 *fftObj,
                                 int maxNumCols)
{
    mSpectrogram = spectrogram;
    mFftObj = fftObj;
    
    mMaxNumCols = maxNumCols;
    
    mViewBarPos = 0.5;
    
    mStartDataPos = 0.0;
    mEndDataPos = 0.0;
    
    Reset();
}

SpectrogramView::~SpectrogramView() {}

void
SpectrogramView::Reset()
{
    mZoomFactor = 1.0;
    
    mAbsZoomFactor = 1.0;
}

void
SpectrogramView::SetData(const vector<WDL_TypedBuf<double> > &channels)
{
    mChannels = channels;
    
    mStartDataPos = 0.0;
    mEndDataPos = 0.0;
    
    int bufferSize = mFftObj->GetBufferSize();
    
    if (!channels.empty())
        mEndDataPos = channels[0].GetSize()/bufferSize;
    
    //UpdateSpectrogramData();
}

void
SpectrogramView::SetViewBarPosition(double pos)
{
    if (mChannels.empty())
      return;
    
    mViewBarPos = pos;
}

void
SpectrogramView::SetViewSelection(double x0, double y0,
                                  double x1, double y1)
{
    mSelection[0] = x0;
    mSelection[1] = y0;
    mSelection[2] = x1;
    mSelection[3] = y1;
    
    mSelectionActive = true;
}

void
SpectrogramView::ClearViewSelection()
{
    mSelectionActive = false;
}

void
SpectrogramView::GetViewDataBounds(long *startDataPos, long *endDataPos)
{
    int bufferSize = mFftObj->GetBufferSize();
    
    *startDataPos = mStartDataPos;
    *endDataPos = mEndDataPos;
}

bool
SpectrogramView::GetDataSelection(long *x0, long *y0, long *x1, long *y1)
{
    if (!mSelectionActive)
        return false;
    
    if (mChannels.empty())
        return false;
    
    int bufferSize = mFftObj->GetBufferSize();
    int overlapping = mFftObj->GetOverlapping();
    
    *x0 = mStartDataPos + mSelection[0]*(mEndDataPos - mStartDataPos + 1);
    
    // Strange...
    *x0 *= overlapping;
    
    // Warning, y is reversed !
    *y0 = (1.0 - mSelection[3])*bufferSize/2; //mChannels[0].GetSize();
    //*y0 /= bufferSize;
    
    *x1 = mStartDataPos + mSelection[2]*(mEndDataPos - mStartDataPos + 1);
    
    // Strange...
    *x1 *= overlapping;
    
    // Warning, y is reversed !
    *y1 = (1.0 - mSelection[1])*bufferSize/2; //mChannels[0].GetSize();
    //*y1 /= bufferSize;
    
    return true;
}

bool
SpectrogramView::UpdateZoomFactor(double zoomChange)
{
    if (mChannels.empty())
        return false;
    
    // Avoid zooming too much
    double prevAbsZoomFactor = mAbsZoomFactor;
    mAbsZoomFactor *= zoomChange;
    
    long channelSize = mChannels[0].GetSize();
    double maxZoom = MAX_ZOOM*((double)channelSize)/100000.0;
    
    if ((mAbsZoomFactor < MIN_ZOOM) || (mAbsZoomFactor > maxZoom/*MAX_ZOOM*/))
    {
        mAbsZoomFactor = prevAbsZoomFactor;
        
        return false;
    }
    
    // Else the zoom is reasonable, set it
    mZoomFactor *= zoomChange;
    
    return true;
}

double
SpectrogramView::GetZoomFactor()
{
    return mZoomFactor;
}

double
SpectrogramView::GetNormZoom()
{
    if (mChannels.empty())
        return 0.0;
    
    long channelSize = mChannels[0].GetSize();
    double maxZoom = MAX_ZOOM*((double)channelSize)/100000.0;

    double result = (mAbsZoomFactor - MIN_ZOOM)/(maxZoom - MIN_ZOOM);
    
    return result;
}

void
SpectrogramView::UpdateSpectrogramData()
{
    double dataZoomFactor = 1.0/mZoomFactor;
    
    mSpectrogram->Reset();
    
    if (mChannels.empty())
        return;
    
    long channelSize = mChannels[0].GetSize();
    if (channelSize == 0)
        return;
    
    int bufferSize = mFftObj->GetBufferSize();
    
    double viewNumLines = mEndDataPos - mStartDataPos + 1.0;
    double dataBarPos = mStartDataPos + mViewBarPos*viewNumLines;
    
    // Recompute data pos
    
    // Use a flag to correct both in case of overdraw bounds
    bool fullView = false;
    
    mStartDataPos = dataBarPos - mViewBarPos*dataZoomFactor*viewNumLines;
    if (mStartDataPos < 0)
    {
        fullView = true;
    }
    
    mEndDataPos = dataBarPos + (1.0 - mViewBarPos)*dataZoomFactor*viewNumLines - 1;
    if (mEndDataPos > channelSize/bufferSize - 1)
    {
        fullView = true;
    }
    
    if (fullView)
    {
        mStartDataPos = 0;
        mEndDataPos = channelSize/bufferSize;
    }
    
    int overlapping = mFftObj->GetOverlapping();
    
    // TODO: manage stereo
    // (for the moment, we manage only mono)
    
    vector<WDL_TypedBuf<double> > dummySc;
    vector<WDL_TypedBuf<double> > dummyOut;
    
    int numChunks = viewNumLines*overlapping;
    if (viewNumLines < 1)
        numChunks = 1;
    
#if !USE_FLOAT_STEP
    int step = numChunks/mMaxNumCols;
    if (step == 0)
        step = 1;
#else
    double step = ((double)numChunks)/mMaxNumCols;
#endif
    
    int pos = mStartDataPos*bufferSize;
    while(pos < mEndDataPos*bufferSize)
    {
        WDL_TypedBuf<double> chunk;
        chunk.Add(&mChannels[0].Get()[pos], bufferSize);
        
        // Fill the rest with zeros if the buffer is too short
        if (chunk.GetSize() < bufferSize)
            Utils::ResizeFillZeros(&chunk, bufferSize);
        
        vector<WDL_TypedBuf<double> > in;
        in.resize(1);
        in[0] = chunk;
            
        mFftObj->Process(in, dummySc, NULL);
            
        pos += step*bufferSize;
    }
    
    mZoomFactor = 1.0;
}
