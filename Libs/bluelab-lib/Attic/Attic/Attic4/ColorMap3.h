//
//  ColorMap3.h
//  BL-Spectrogram
//
//  Created by Pan on 08/04/18.
//
//

#ifndef __BL_Spectrogram__ColorMap3__
#define __BL_Spectrogram__ColorMap3__

#include "IPlug_include_in_plug_hdr.h"

//#define COLORMAP_GAMMA 2.2

#define COLORMAP_GAMMA 3.0

// ColorMap3: for BLSpectrogram3
// - use shader !

class ColorMap3
{
public:
    typedef unsigned int CmColor;
    
    ColorMap3(const unsigned char startColor[4],
              const unsigned char endColor[4],
              BL_FLOAT blackLimit = 0.5, BL_FLOAT whiteLimit = -1.0);
    
    virtual ~ColorMap3();
    
    void SetRange(BL_FLOAT range);
    
    void SetContrast(BL_FLOAT contrast);
    
    void GetColor(BL_FLOAT t, CmColor *color);
    
    void SavePPM(const char *fileName);
    
    void GetDataRGBA(WDL_TypedBuf<CmColor> *result);
    
protected:
    void Generate(const unsigned char startColor[4],
                  const unsigned char endColor[4],
                  WDL_TypedBuf<CmColor> *result);
    
    // Take 3 zones: from black to first color,
    // and from first color to second color
    void Generate2(const unsigned char startColor[4],
                   const unsigned char endColor[4],
                   WDL_TypedBuf<CmColor> *result);
    
    // Take 4 zones: from black to first color,
    // from first color to second color,
    // and from second color to white if specified
    void Generate3(const unsigned char startColor[4],
                   const unsigned char endColor[4],
                   WDL_TypedBuf<CmColor> *result);
    
    unsigned char mStartColor[4];
    unsigned char mEndColor[4];
    
    BL_FLOAT mRange;
    BL_FLOAT mContrast;
    
    BL_FLOAT mBlackLimit;
    
    // Interpolate from second color to white ?
    // Set to -1 to disable
    BL_FLOAT mWhiteLimit;
    
    WDL_TypedBuf<CmColor> mColors;
    
    // For optimization
    long mSize;
    CmColor *mBuf;
};

#endif /* defined(__BL_Spectrogram__ColorMap__) */
