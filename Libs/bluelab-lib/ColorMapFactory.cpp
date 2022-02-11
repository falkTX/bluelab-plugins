//
//  ColorMapFactory.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 4/10/20.
//
//

#include <ColorMap4.h>

#include "ColorMapFactory.h"

ColorMapFactory::ColorMapFactory(bool useGLSL,
                                 bool transparentColorMap)
{
    mUseGLSL = useGLSL;
    
    mTransparentColorMap = transparentColorMap;
}

ColorMapFactory::~ColorMapFactory() {}

ColorMap4 *
ColorMapFactory::CreateColorMap(ColorMap colorMapId)
{
    // Methods to create a good colormap
    //
    // Method 1:
    // - take wasp (export is as ppm)
    // - rescale it to length 128
    // - change hue
    // - pick colors at 1 (for 0.25), 14 (for 0.5) and 40 (for 0.75)
    // color at 0.0 is 0, color at 1.0 is 255
    // - eventually set the same color as wasp for 0.25 (if the new color looks too dirty)
    //
    // Method 2:
    // - get the t values from the 128 png colormap image
    // - compute t^1/3, and put these values instead of t directly
    // => works very well!
    //
    
    ColorMap4 *result = NULL;
    
    switch(colorMapId)
    {
        case COLORMAP_BLUE:
        {
            
            // Blue and dark pink
            result = new ColorMap4(mUseGLSL);
            
            if (mTransparentColorMap)
                result->AddColor(0, 0, 0, 0, 0.0);
            else
                result->AddColor(0, 0, 0, 255, 0.0);

            
            result->AddColor(68, 22, 68, 255, 0.25);
            result->AddColor(32, 122, 190, 255, 0.5);
            result->AddColor(172, 212, 250, 255, 0.75);
            
            result->AddColor(255, 255, 255, 255, 1.0);
        }
        break;
            
        case COLORMAP_DAWN:
        {
            
            // Dawn (thermal...)
            result = new ColorMap4(mUseGLSL);
            
            result->AddColor(4, 35, 53, 255, 0.0);
            
            result->AddColor(90, 61, 154, 255, 0.25);
            result->AddColor(185, 97, 125, 255, 0.5);
            result->AddColor(245, 136, 71, 255, 0.75);
            
            result->AddColor(233, 246, 88, 255, 1.0);
        }
        break;
        
        case COLORMAP_DAWN_FIXED:
        {
            
            // Dawn (thermal...), fixed
            result = new ColorMap4(mUseGLSL);
            
            result->AddColor(4, 35, 52, 255, 0.0);
            
            result->AddColor(62, 51, 159, 255, /*0.2*/0.588);
            result->AddColor(139, 83, 140, 255, /*0.4*/0.739);
            result->AddColor(213, 107, 109, 255, /*0.6*/0.844);
            result->AddColor(251, 166, 60, 255, /*0.8*/0.929);
            
            //result->AddColor(4, 35, 52, 255, 0.25);
            //result->AddColor(19, 50, 115, 255, 0.5);
            //result->AddColor(109, 70, 147, 255, 0.75);
            
            result->AddColor(232, 249, 90, 255, 1.0);
        }
        break;
            
        case COLORMAP_RAINBOW:
        {
            // Multicolor ("jet")
            //
            // Exponential
            // Matches better with the other colormaps for a given brightness/contrast setup.
            //
            result = new ColorMap4(mUseGLSL);
            
            result->AddColor(0, 0, 128, 255, 0.0);
            result->AddColor(0, 0, 246, 255, 0.1);
            result->AddColor(0, 77, 255, 255, 0.2);
            result->AddColor(0, 177, 255, 255, 0.3);
            result->AddColor(38, 255, 209, 255, 0.4);
            result->AddColor(125, 255, 122, 255, 0.5);
            result->AddColor(206, 255, 40, 255, 0.6);
            result->AddColor(255, 200, 0, 255, 0.7);
            result->AddColor(255, 100, 0, 255, 0.8);
            result->AddColor(241, 8, 0, 255, 0.9);
            result->AddColor(132, 0, 0, 255, 1.0);
        }
        break;
        
        case COLORMAP_RAINBOW_LINEAR:
        {
            // Multicolor ("jet")
            //
            // Linear
            //
            result = new ColorMap4(mUseGLSL);
            
            result->AddColor(0, 0, 128, 255, 0.0);
            result->AddColor(0, 0, 246, 255, 0.467);
            result->AddColor(0, 77, 255, 255, 0.587);
            result->AddColor(0, 177, 255, 255, 0.672);
            result->AddColor(38, 255, 209, 255, 0.739);
            result->AddColor(125, 255, 122, 255, 0.795);
            result->AddColor(206, 255, 40, 255, 0.844);
            result->AddColor(255, 200, 0, 255, 0.889);
            result->AddColor(255, 100, 0, 255, 0.929);
            result->AddColor(241, 8, 0, 255, 0.965);
            result->AddColor(132, 0, 0, 255, 1.0);
        }
        break;

        case COLORMAP_PURPLE:
        {
            // Purple, derived from Wasp
            result = new ColorMap4(mUseGLSL);
            
            if (mTransparentColorMap)
                result->AddColor(0, 0, 0, 0, 0.0);
            else
                result->AddColor(0, 0, 0, 255, 0.0);
            
            //result->AddColor(48, 65, 21, 255, 0.25); // Dirty green duck
            result->AddColor(21, 34, 65, 255, 0.25); // From wasp: GOOD !
            result->AddColor(40, 33, 187, 255, 0.5);
            result->AddColor(173, 98, 250, 255, 0.75);
            
            result->AddColor(255, 255, 255, 255, 1.0);
        }
        break;
        
        case COLORMAP_SWEET:
        {
            // Cyan and Pink ("Sweet")
            result = new ColorMap4(mUseGLSL);
            
            if (mTransparentColorMap)
                result->AddColor(0, 0, 0, 0, 0.0);
            else
                result->AddColor(0, 0, 0, 255, 0.0);
            
            result->AddColor(0, 150, 150, 255, 0.2);//
            result->AddColor(0, 255, 255, 255, 0.5/*0.2*/);
            result->AddColor(255, 0, 255, 255, 0.8);
            
            result->AddColor(255, 255, 255, 255, 1.0);
        }
        break;

        case COLORMAP_OCEAN:
        {
            // New Blue
            result = new ColorMap4(mUseGLSL);
            
            if (mTransparentColorMap)
                result->AddColor(0, 0, 0, 0, 0.0);
            else
                result->AddColor(0, 0, 0, 255, 0.0);
            
            //result->AddColor(68, 22, 68, 255, 0.25); // Origin, from Blue
            //result->AddColor(42, 14, 42, 255, 0.25); // NEW: darker => better contrast
            //result->AddColor(55, 18, 55, 255, 0.25); // NEW2: middle contrast
            result->AddColor(50, 16, 50, 255, 0.25); // NEW2: middle contrast
            
            //result->AddColor(41, 130, 199, 255, 0.5); // Misses contrast
            result->AddColor(31, 108, 161, 255, 0.5); // NEW: good!
            //result->AddColor(36, 177, 188, 255, 0.5); // NEW2: more green and more clear (bad)
            result->AddColor(103, 130, 249, 255, 0.75);
            
            result->AddColor(255, 255, 255, 255, 1.0);
        }
        break;
            
        case COLORMAP_WASP:
        {
            // Cyan and Orange (Wasp)
            result = new ColorMap4(mUseGLSL);
            
            if (mTransparentColorMap)
                result->AddColor(0, 0, 0, 0, 0.0);
            else
                result->AddColor(0, 0, 0, 255, 0.0);
            
            result->AddColor(22, 35, 68, 255, 0.25);
            result->AddColor(190, 90, 32, 255, 0.5);
            result->AddColor(250, 220, 96, 255, 0.75);
            
            result->AddColor(255, 255, 255, 255, 1.0);
        }
        break;
            
        case COLORMAP_GREEN:
        {
            // Green
            result = new ColorMap4(mUseGLSL);
            result->AddColor(32, 42, 26, 255, 0.0);
            result->AddColor(66, 108, 60, 255, 0.25);
            result->AddColor(98, 150, 82, 255, 0.5);
            result->AddColor(166, 206, 148, 255, 0.75);
            result->AddColor(255, 255, 255, 255, 1.0);
        }
        break;
        
        case COLORMAP_GREY:
        {
            // Grey
            result = new ColorMap4(mUseGLSL);
            result->AddColor(0, 0, 0, 255, 0.0);
            result->AddColor(128, 128, 128, 255, 0.5);
            result->AddColor(255, 255, 255, 255, 1.0);
        }
        break;
            
        case COLORMAP_GREEN_RED:
        {
            // Green and red
            result = new ColorMap4(mUseGLSL);
            result->AddColor(0, 0, 0, 255, 0.0);
            result->AddColor(0, 255, 0, 255, 0.25);
            result->AddColor(255, 0, 0, 255, 1.0);
        }
        break;
        
        case COLORMAP_SWEET2:
        {
            // Cyan to orange
            result = new ColorMap4(mUseGLSL);
            result->AddColor(0, 0, 0, 255, 0.0);
            result->AddColor(0, 255, 255, 255, 0.25);
            //result->AddColor(255, 128, 0, 255, 1.0);
            result->AddColor(255, 128, 0, 255, 0.75);
            result->AddColor(255, 228, 130, 255, 1.0);
        }
        break;
        
        case COLORMAP_SKY:
        {
#if 0
            // Sky (parula / ice)
            result = new ColorMap4(mUseGLSL);
            result->AddColor(0, 0, 0, 255, 0.0);
            
            result->AddColor(62, 38, 168, 255, 0.004);
            
            result->AddColor(46, 135, 247, 255, 0.25);
            result->AddColor(11, 189, 189, 255, 0.5);
            result->AddColor(157, 201, 67, 255, 0.75);
            
            result->AddColor(249, 251, 21, 255, 1.0);
#endif
            
            // Not so bad, quite similar to original ice
            //
            // Sky (ice)
            result = new ColorMap4(mUseGLSL);
            result->AddColor(4, 6, 19, 255, 0.0);
            
            result->AddColor(58, 61, 126, 255, 0.25);
            result->AddColor(67, 126, 184, 255, 0.5);
            
            //result->AddColor(112, 182, 205, 255, 0.75);
            result->AddColor(73, 173, 208, 255, 0.75);
            
            result->AddColor(232, 251, 252, 255, 1.0);
        }
        break;
            
        case COLORMAP_LANDSCAPE:
        {
            // Landscape (terrain)
            result = new ColorMap4(mUseGLSL);
            result->AddColor(51, 51, 153, 255, 0.0);
            result->AddColor(2, 148, 250, 255, 0.150);
            result->AddColor(37, 111, 109, 255, 0.286);
            result->AddColor(253, 254, 152, 255, 0.494);
            result->AddColor(128, 92, 84, 255, 0.743);
            result->AddColor(255, 255, 255, 255, 1.0);
        }
        break;
            
        case COLORMAP_FIRE:
        {
            // Fire (fire)
            result = new ColorMap4(mUseGLSL);
            result->AddColor(0, 0, 0, 255, 0.0);
            result->AddColor(143, 0, 0, 255, 0.200);
            result->AddColor(236, 0, 0, 255, 0.337);
            result->AddColor(255, 116, 0, 255, 0.535);
            result->AddColor(255, 234, 0, 255, 0.706);
            result->AddColor(255, 255, 255, 255, 1.0);
        }
        break;

        // NEW
        case COLORMAP_PURPLE2:
        {
            // Purple, derived from Wasp
            result = new ColorMap4(mUseGLSL);
            
            if (mTransparentColorMap)
                result->AddColor(0, 0, 0, 0, 0.0);
            else
                result->AddColor(0, 0, 0, 255, 0.0);
            
            //result->AddColor(21, 34, 65, 255, 0.25);
            //result->AddColor(65, 21, 34, 255, 0.25); // Blood red
            result->AddColor(25, 8, 17, 255, 0.25);
            
            //result->AddColor(40, 33, 187, 255, 0.5);
            result->AddColor(73, 0, 148, 255, 0.5);
 
            //result->AddColor(173, 98, 250, 255, 0.75);
            result->AddColor(118, 89, 254, 255, 0.75);
            
            result->AddColor(255, 255, 255, 255, 1.0);
        }
        break;
        
        default:
            break;
    }
    
    if (result != NULL)
    {
        result->Generate();
        
#if 0 // DEBUG
        char debugFileName[255];
        sprintf(debugFileName, "colormap-%d.ppm", colorMapId);
        result->SavePPM(debugFileName);
#endif
    }
    
    return result;
}
