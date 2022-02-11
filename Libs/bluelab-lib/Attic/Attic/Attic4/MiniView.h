//
//  MiniView.h
//  BL-Ghost
//
//  Created by Pan on 15/06/18.
//
//

#ifndef __BL_Ghost__MiniView__
#define __BL_Ghost__MiniView__

#include "IPlug_include_in_plug_hdr.h"

class NVGcontext;

class MiniView
{
public:
    MiniView(int maxNumPoints,
             BL_FLOAT x0, BL_FLOAT y0, BL_FLOAT x1, BL_FLOAT y1);
    
    virtual ~MiniView();
  
    void Display(NVGcontext *vg, int width, int height);
  
    bool IsPointInside(int x, int y, int width, int height);
    
    void SetData(const WDL_TypedBuf<BL_FLOAT> &data);
    
    void GetWaveForm(WDL_TypedBuf<BL_FLOAT> *waveform);
    
    void SetBounds(BL_FLOAT minNormX, BL_FLOAT maxNormX);
    
    BL_FLOAT GetDrag(int dragX, int width);
    
protected:
    BL_FLOAT mBounds[4];
    
    int mMaxNumPoints;
    
    WDL_TypedBuf<BL_FLOAT> mWaveForm;
    
    BL_FLOAT mMinNormX;
    BL_FLOAT mMaxNormX;
};

#endif /* defined(__BL_Ghost__MiniView__) */
