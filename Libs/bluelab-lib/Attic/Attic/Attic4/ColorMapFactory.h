//
//  ColorMapFactory.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 4/10/20.
//
//

#ifndef __BL_SoundMetaViewer__ColorMapFactory__
#define __BL_SoundMetaViewer__ColorMapFactory__

class ColorMap4;
class ColorMapFactory
{
public:
    enum ColorMap
    {
      COLORMAP_BLUE = 0,
      COLORMAP_DAWN,
      COLORMAP_DAWN_FIXED, //
      COLORMAP_RAINBOW,
      COLORMAP_RAINBOW_LINEAR,
      COLORMAP_PURPLE,
      COLORMAP_SWEET,
      COLORMAP_OCEAN,
      COLORMAP_WASP,
        
      COLORMAP_GREEN,
      COLORMAP_GREY,
      COLORMAP_GREEN_RED,
      COLORMAP_SWEET2,
      COLORMAP_SKY,
      COLORMAP_LANDSCAPE,
      COLORMAP_FIRE
    };
    
    ColorMapFactory(bool useGLSL, bool transparentColorMap);
    
    virtual ~ColorMapFactory();
    
    ColorMap4 *CreateColorMap(ColorMap colorMapId);
    
protected:
    bool mUseGLSL;
    bool mTransparentColorMap;
};

#endif /* defined(__BL_SoundMetaViewer__ColorMapFactory__) */
