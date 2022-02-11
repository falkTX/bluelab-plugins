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
    
    void Reset();

    
    void SetData(const vector<WDL_TypedBuf<double> > &channels);
    
    void SetViewBarPosition(double pos);
    
    void SetViewSelection(double x0, double y0,
                          double x1, double y1);
    
    void ClearViewSelection();
    
    void GetViewDataBounds(long *startDataPos, long *endDataPos);
    
    bool GetDataSelection(long *x0, long *y0, long *x1, long *y1);
    
    // Return true if the zoom had actually be done
    // (otherwise, we can have reached the zoom limits)
    bool UpdateZoomFactor(double zoomChange);
    double GetZoomFactor();
    
    // Between 0 and 1 => for zoom between min and max
    double GetNormZoom();
    
    void UpdateSpectrogramData();
    
protected:
    BLSpectrogram3 *mSpectrogram;
    int mMaxNumCols;
    
    FftProcessObj14 *mFftObj;
    
    vector<WDL_TypedBuf<double> > mChannels;
    
    // Use double => avoid small translations due to rounging
    double mViewBarPos;
    double mStartDataPos;
    double mEndDataPos;
    
    double mZoomFactor;
    
    // Total zoom factor
    double mAbsZoomFactor;
    
    // View selection
    bool mSelectionActive;
    double mSelection[4];
};

#endif /* defined(__BL_Ghost__SpectrogramView__) */
