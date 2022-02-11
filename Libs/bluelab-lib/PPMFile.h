//
//  PPMFile.h
//  BL-Spectrogram
//
//  Created by Pan on 09/04/18.
//
//

#ifndef __BL_Spectrogram__PPMFile__
#define __BL_Spectrogram__PPMFile__

#include <BLTypes.h>

class PPMFile
{
public:

    // Types
    typedef struct
    {
        unsigned char red, green, blue;
    } PPMPixel;

    typedef struct
    {
        unsigned short red, green, blue;
    } PPMPixel16;


    typedef struct _PPMImage
    {
        int w, h;
        
        PPMPixel *data;
        PPMPixel16 *data16;
    } PPMImage;

    
    // Methods
    static PPMImage *ReadPPM(const char *filename);
    
    static PPMImage *ReadPPM16(const char *filename);
    
    static void SavePPM(const char *filename, BL_FLOAT *image,
                        int width, int height, int bpp,
                        BL_FLOAT colorCoeff, bool deInterlace = false);
};


#endif /* defined(__BL_Spectrogram__PPMFile__) */
