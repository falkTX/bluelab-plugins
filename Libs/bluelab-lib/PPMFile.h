/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
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
