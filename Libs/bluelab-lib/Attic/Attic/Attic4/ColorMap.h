//
//  ColorMap.h
//  BL-Spectrogram
//
//  Created by Pan on 08/04/18.
//
//

#ifndef __BL_Spectrogram__ColorMap__
#define __BL_Spectrogram__ColorMap__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class ColorMap
{
public:
    ColorMap(const unsigned char startColor[3],
             const unsigned char endColor[3]);
    
    virtual ~ColorMap();
    
    void GetColor(BL_FLOAT t, unsigned char color[3]);
    
    void SavePPM(const char *fileName);
    
protected:
    typedef struct
    {
        unsigned char rgb[3];
    } Color;
    
    void Generate(const unsigned char startColor[3],
                  const unsigned char endColor[3],
                  WDL_TypedBuf<Color> *result);
    
    WDL_TypedBuf<Color> mColors;
};

#endif /* defined(__BL_Spectrogram__ColorMap__) */
