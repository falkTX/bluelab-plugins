//
//  ColorMap2.h
//  BL-Spectrogram
//
//  Created by Pan on 08/04/18.
//
//

#ifndef __BL_Spectrogram__ColorMap2__
#define __BL_Spectrogram__ColorMap2__

#include "IPlug_include_in_plug_hdr.h"

//#define COLORMAP_GAMMA 2.2
#define COLORMAP_GAMMA 3.0

class ColorMap2
{
public:
    typedef unsigned int CmColor;
    
    ColorMap2(const unsigned char startColor[4],
              const unsigned char endColor[4]);
    
    virtual ~ColorMap2();
    
    void SetRange(BL_FLOAT range);
    
    void SetContrast(BL_FLOAT contrast);
    
    void GetColor(BL_FLOAT t, CmColor *color);
    
    void SavePPM(const char *fileName);
    
protected:
    void Generate(const unsigned char startColor[4],
                  const unsigned char endColor[4],
                  WDL_TypedBuf<CmColor> *result);
    
    unsigned char mStartColor[4];
    unsigned char mEndColor[4];
    
    BL_FLOAT mRange;
    BL_FLOAT mContrast;
    
    WDL_TypedBuf<CmColor> mColors;
    
    // For optimization
    long mSize;
    CmColor *mBuf;
};

#endif /* defined(__BL_Spectrogram__ColorMap__) */
