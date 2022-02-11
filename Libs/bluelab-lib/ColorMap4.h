//
//  ColorMap4.h
//  BL-Spectrogram
//
//  Created by Pan on 08/04/18.
//
//

#ifndef __BL_Spectrogram__ColorMap4__
#define __BL_Spectrogram__ColorMap4__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

#include <BLTypes.h>

//#define COLORMAP_GAMMA 2.2
#define COLORMAP_GAMMA 3.0

// ColorMap3: for BLSpectrogram3
// - use shader !
//
// ColorMap4: can define seleval colors and associated parameter

class ColorMap4
{
public:
    typedef unsigned int CmColor;
    
    ColorMap4(bool glsl);
    
    virtual ~ColorMap4();
    
    void AddColor(unsigned char r, unsigned char g,
                  unsigned char b, unsigned char a,
                  BL_FLOAT t);
    
    void Generate();
    
    void SetRange(BL_FLOAT range);
    BL_FLOAT GetRange();
    
    void SetContrast(BL_FLOAT contrast);
    BL_FLOAT GetContrast();
    
    void GetColor(BL_FLOAT t, CmColor *color);
    
    void SavePPM(const char *fileName);
    
    void GetDataRGBA(WDL_TypedBuf<CmColor> *result);
    
protected:
    void Generate(WDL_TypedBuf<CmColor> *result);
    
    vector<CmColor> mColors;
    vector<BL_FLOAT> mTs;
    
    BL_FLOAT mRange;
    BL_FLOAT mContrast;
    
    WDL_TypedBuf<CmColor> mResultColors;
    
    // For optimization
    long mSize;
    CmColor *mBuf;
    
    bool mUseGLsl;
};

#endif /* defined(__BL_Spectrogram__ColorMap__) */
