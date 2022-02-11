//
//  FillTriangle.h
//  BL-DUET
//
//  Created by applematuer on 5/6/20.
//
//

#ifndef __BL_DUET__FillTriangle__
#define __BL_DUET__FillTriangle__

#include "IPlug_include_in_plug_hdr.h"

// See: http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
template <typename T>
class FillTriangle
{
public:
    static void Fill(int width, int height, WDL_TypedBuf<T> *image,
                     int v0[2], int v1[2], int v2[2], T col);
    
protected:
    static void FillBottomFlatTriangle(int width, int height, WDL_TypedBuf<T> *image,
                                       int v0[2], int v1[2], int v2[2], T col);
    
    static void FillTopFlatTriangle(int width, int height, WDL_TypedBuf<T> *image,
                                    int v0[2], int v1[2], int v2[2], T col);
    
    static void SortVerticesAscendingByY(int v0[2], int v1[2], int v2[2]);
    
    static void DrawScanLine(int width, int height, WDL_TypedBuf<T> *image,
                             int x0, int x1, int y, T col);

};

#endif /* defined(__BL_DUET__FillTriangle__) */
