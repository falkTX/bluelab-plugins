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
//  Axis3D.h
//  BL-Waves
//
//  Created by applematuer on 2/10/19.
//
//

#ifndef __BL_Waves__Axis3D__
#define __BL_Waves__Axis3D__

#include <vector>
#include <string>
using namespace std;

#include <BLTypes.h>

// Interface
class PointProjector
{
public:
    virtual void ProjectPoint(BL_FLOAT projP[3], const BL_FLOAT p[3],
                              int width, int height) = 0;
};

class PointProjectorFloat
{
public:
    virtual void ProjectPoint(float projP[3], const float p[3],
                              int width, int height) = 0;
};

struct NVGcontext;

// Axis3D class
class Axis3D
{
public:
    Axis3D(char *labels[], BL_FLOAT labelNormPos[],
           int numData, BL_FLOAT p0[3], BL_FLOAT p1[3]);
    
    virtual ~Axis3D();
    
    void UpdateLabels(char *labels[], BL_FLOAT labelNormPos[],
                      int numData, BL_FLOAT p0[3], BL_FLOAT p1[3]);
    
    // For TimeAxis3D
    void UpdateLabels(char *labels[], BL_FLOAT labelNormPos[], int numData);
    
    void SetPointProjector(PointProjector *projector);
    void SetPointProjector(PointProjectorFloat *projector);
    
    void SetDoOverlay(bool flag);
    
    void SetColor(unsigned char color[4]);
    void SetLabelColor(unsigned char labelColor[4]);
    
    void SetOverColor(unsigned char color[4]);
    void SetOverLabelColor(unsigned char labelColor[4]);
    
    void Draw(NVGcontext *vg, int width, int height);
    
    void SetScale(BL_FLOAT scale);
    
    void SetDBScale(bool flag, BL_FLOAT minDB);
    
protected:
    // Axis
    typedef struct
    {
        BL_FLOAT mT;
        string mText;
    } AxisData;
    
    void DoDraw(NVGcontext *vg,
                int width, int height,
                unsigned char color[4],
                unsigned char labelColor[4],
                int overlayOffset);

    void DrawText(NVGcontext *vg, BL_FLOAT x, BL_FLOAT y, BL_FLOAT fontSize,
                  const char *text, unsigned char color[4],
                  int halign, int valign);
    
    //
    bool mDoOverlay;
    
    BL_FLOAT mP0[3];
    BL_FLOAT mP1[3];
    
    unsigned char mColor[4];
    unsigned char mLabelColor[4];
    
    // Over color is in fact the "under" color
    unsigned char mOverColor[4];
    unsigned char mOverLabelColor[4];
    
    vector<AxisData> mValues;
    
    PointProjector *mProjector;
    PointProjectorFloat *mProjectorFloat;
    
    BL_FLOAT mScale;
    
    //
    bool mDBScale;
    BL_FLOAT mMinDB;

};

#endif /* defined(__BL_Waves__Axis3D__) */
