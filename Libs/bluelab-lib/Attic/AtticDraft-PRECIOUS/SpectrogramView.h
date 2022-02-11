//
//  SpectrogramView.h
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#ifndef __BL_Ghost__SpectrogramView__
#define __BL_Ghost__SpectrogramView__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class BLSpectrogram3;
class FftProcessObj14;

class SpectrogramView
{
public:
    SpectrogramView(BLSpectrogram3 *spectrogram,
                    FftProcessObj14 *fftObj,
                    int maxNumCols);
    
    virtual ~SpectrogramView();
    
    void SetData(const vector<WDL_TypedBuf<double> > &channels);
    
    void SetPosition(double pos);
    
    double GetPosition();
    
    void SetSamplePosition(long samplePos);
    
    void SetZoomFactor(double zoomFactor);
    
    void UpdateZoomFactor(double zoomChange);
    double GetZoomFactor();
    
    void SetDataZoomFactor(double zoomChange);
    double GetDataZoomFactor();
    
    void UpdateSpectrogramData();
    
protected:
    BLSpectrogram3 *mSpectrogram;
    int mMaxNumCols;
    
    FftProcessObj14 *mFftObj;
    
    vector<WDL_TypedBuf<double> > mChannels;
    
    double mCenterPos;
    
    long mSamplePos;
    long mStartPos;
    long mEndPos;
    
    double mZoomFactor;
    double mDataZoomFactor;
};

#endif /* defined(__BL_Ghost__SpectrogramView__) */
