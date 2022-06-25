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
//  FillTriangle.cpp
//  BL-DUET
//
//  Created by applematuer on 5/6/20.
//
//

//#include "FillTriangle.h"

//#define INF 1e15
#include <BLUtils.h>
#include <BLUtilsMath.h>

template <typename T>
class FillTriangle
{
public:
    static void
    Fill(int width, int height, WDL_TypedBuf<T> *image,
         int v0[2], int v1[2], int v2[2], T col)
    {
        // Degerated case, all points on the same y
        if ((v1[1] == v0[1]) && (v2[1] == v0[1]))
        {
            int minX = BL_INFI;
            int maxX = -BL_INFI;
        
            if (v0[0] < minX)
                minX = v0[0];
            if (v1[0] < minX)
                minX = v1[0];
            if (v2[0] < minX)
                minX = v2[0];
        
            if (v0[0] > maxX)
                maxX = v0[0];
            if (v1[0] > maxX)
                maxX = v1[0];
            if (v2[0] > maxX)
                maxX = v2[0];
        
            DrawScanLine(width, height, image, minX, maxX, v0[1], col);
        
            return;
        }
    
        // At first sort the three vertices by y-coordinate ascending
        // so v0 is the topmost vertice.
        SortVerticesAscendingByY(v0, v1, v2);
    
        // Here we know that v0.y <= v1.y <= v2.y
        // check for trivial case of bottom-flat triangle
        if (v1[1] == v2[1])
        {
            FillBottomFlatTriangle(width, height, image, v0, v1, v2, col);
        }
        // Check for trivial case of top-flat triangle.
        else if (v0[1] == v1[1])
        {
            FillTopFlatTriangle(width, height, image, v0, v1, v2, col);
        }
        else
        {
            // General case - split the triangle in a topflat and bottom-flat one
            int v3[2];
            v3[0] = (int)(v0[0] + ((BL_FLOAT)(v1[1] - v0[1]) / (BL_FLOAT)(v2[1] - v0[1])) * (v2[0] - v0[0]));
        
            v3[1] = v1[1];
        
            FillBottomFlatTriangle(width, height, image, v0, v1, v3, col);
            FillTopFlatTriangle(width, height, image, v1, v3, v2, col);
        }
    }

protected:
    static void
    FillBottomFlatTriangle(int width, int height, WDL_TypedBuf<T> *image,
                           int v0[2], int v1[2], int v2[2], T col)
    {
        BL_FLOAT invslope1 = ((BL_FLOAT)(v1[0] - v0[0])) / (v1[1] - v0[1]);
        BL_FLOAT invslope2 = ((BL_FLOAT)(v2[0] - v0[0])) / (v2[1] - v0[1]);
    
        BL_FLOAT curx1 = v0[0];
        BL_FLOAT curx2 = v0[0];
    
        for (int scanlineY = v0[1]; scanlineY <= v1[1]; scanlineY++)
        {
            DrawScanLine(width, height, image,
                         (int)curx1, (int)curx2, scanlineY, col);
        
            curx1 += invslope1;
            curx2 += invslope2;
        }
    }

    static void
    FillTopFlatTriangle(int width, int height, WDL_TypedBuf<T> *image,
                        int v0[2], int v1[2], int v2[2], T col)
    {
        BL_FLOAT invslope1 = ((BL_FLOAT)(v2[0] - v0[0])) / (v2[1] - v0[1]);
        BL_FLOAT invslope2 = ((BL_FLOAT)(v2[0] - v1[0])) / (v2[1] - v1[1]);
    
        BL_FLOAT curx1 = v2[0];
        BL_FLOAT curx2 = v2[0];
    
        for (int scanlineY = v2[1]; scanlineY > v0[1]; scanlineY--)
        {
            DrawScanLine(width, height, image,
                         (int)curx1, (int)curx2, scanlineY, col);
        
            curx1 -= invslope1;
            curx2 -= invslope2;
        }
    }

    static void
    SortVerticesAscendingByY(int v0[2], int v1[2], int v2[2])
    {
        // This sort method is a bit weird...
        //
    
        int triangle[3][2] = { { v0[0], v0[1] }, { v1[0], v1[1] }, { v2[0], v2[1] } };
    
        int indices[3] = { -1, -1, -1 };
        int minY = BL_INFI;
        int maxY = -BL_INFI;
    
        // Find min and max Y and set the corresponding indices
        for (int i = 0; i < 3; i++)
        {
            if (triangle[i][1] < minY)
            {
                minY = triangle[i][1];
                indices[0] = i;
            }
        
            if (triangle[i][1] > maxY)
            {
                maxY = triangle[i][1];
                indices[2] = i;
            }
        }
    
        // Find the missing index
        for (int i = 0; i < 3; i++)
        {
            if ((indices[0] == i) || (indices[2] == i))
                continue;
        
            indices[1] = i;
            break;
        }
    
        // Set the result
        for (int i = 0; i < 2; i++)
        {
            v0[i] = triangle[indices[0]][i];
            v1[i] = triangle[indices[1]][i];
            v2[i] = triangle[indices[2]][i];
        }
    }

    static void
    DrawScanLine(int width, int height, WDL_TypedBuf<T> *image,
                 int x0, int x1, int y, T col)
    {
        if ((y < 0) || (y > height - 1))
            return;
    
        // Swap if necessary
        if (x0 > x1)
        {
            int tmp = x0;
            x0 = x1;
            x1 = tmp;
        }
    
        int yoffset = y*width;
        for (int i = x0; i <= x1; i++)
        {
            if ((i < 0) || (i > width - 1))
                continue;
        
            image->Get()[i + yoffset] = col;
        }
    }
};
