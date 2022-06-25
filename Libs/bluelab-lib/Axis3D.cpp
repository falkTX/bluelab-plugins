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
//  Axis3D.cpp
//  BL-Waves
//
//  Created by applematuer on 2/10/19.
//
//

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>
#include <GraphSwapColor.h>
#include <BLUtils.h>

#include "Axis3D.h"


#define EPS 1e-15

// 1 pixel
#define OVERLAY_OFFSET -1.0

//#define FONT "font"
#define FONT "font-bold"

Axis3D::Axis3D(char *labels[], BL_FLOAT labelNormPos[], int numData,
               BL_FLOAT p0[3], BL_FLOAT p1[3])
{
    mProjector = NULL;
    mProjectorFloat = NULL;
    
    mDoOverlay = false;
    
    // Default colors (as in Ghost-X)
    //
    int axisColor[4] = { 48, 48, 48, 255 };
    int axisLabelColor[4] = { 255, 255, 255, 255 };
    int axisOverlayColor[4] = { 48, 48, 48, 255 };
    
    for (int i = 0; i < 4; i++)
    {
        mColor[i] = axisColor[i];
        mLabelColor[i] = axisLabelColor[i];
     
        mOverColor[i] = axisOverlayColor[i];
        mOverLabelColor[i] = axisOverlayColor[i];
    }
    
    UpdateLabels(labels, labelNormPos, numData, p0, p1);
    
    mScale = 1.0;
    
    mDBScale = false;
    mMinDB = -60.0;
}

Axis3D::~Axis3D() {}

void
Axis3D::UpdateLabels(char *labels[], BL_FLOAT labelNormPos[],
                     int numData, BL_FLOAT p0[3], BL_FLOAT p1[3])
{
    for (int i = 0; i < 3; i++)
    {
        mP0[i] = p0[i];
        mP1[i] = p1[i];
    }
    
    UpdateLabels(labels, labelNormPos, numData);
}

void
Axis3D::UpdateLabels(char *labels[], BL_FLOAT labelNormPos[], int numData)
{
    mValues.clear();
    
    // Copy data
    for (int i = 0; i < numData; i++)
    {
        char *label = labels[i];
        BL_FLOAT t = labelNormPos[i];
        
        // Sqeeze when t > 1.
        // It means for example that we want to display "80KHz",
        // and the sample rate is just 44100Hz
        if (t > 1.0)
            continue;
        
        string text(label);
        
        AxisData aData;
        aData.mT = t;
        aData.mText = text;
        
        mValues.push_back(aData);
    }
}

void
Axis3D::SetPointProjector(PointProjector *projector)
{
    mProjector = projector;
}

void
Axis3D::SetPointProjector(PointProjectorFloat *projector)
{
    mProjectorFloat = projector;
}

void
Axis3D::SetDoOverlay(bool flag)
{
    mDoOverlay = flag;
}

void
Axis3D::SetColor(unsigned char color[4])
{
    for (int i = 0; i < 4; i++)
        mColor[i] = color[i];
}

void
Axis3D::SetLabelColor(unsigned char color[4])
{
    for (int i = 0; i < 4; i++)
        mLabelColor[i] = color[i];
}

void
Axis3D::SetOverColor(unsigned char color[4])
{
    mDoOverlay = true;
    
    for (int i = 0; i < 4; i++)
        mOverColor[i] = color[i];
}

void
Axis3D::SetOverLabelColor(unsigned char color[4])
{
    mDoOverlay = true;
    
    for (int i = 0; i < 4; i++)
        mOverLabelColor[i] = color[i];
}

void
Axis3D::Draw(NVGcontext *vg, int width, int height)
{
    if (!mDoOverlay)
    {
        DoDraw(vg, width, height, mColor, mLabelColor, 0);
    }
    else
    {
        DoDraw(vg, width, height, mOverColor, mOverLabelColor, 0);
        
        DoDraw(vg, width, height, mColor, mLabelColor, OVERLAY_OFFSET);
    }
}

void
Axis3D::SetScale(BL_FLOAT scale)
{
    mScale = scale;
}

void
Axis3D::SetDBScale(bool flag, BL_FLOAT minDB)
{
    mDBScale = flag;
    mMinDB = minDB;
}

void
Axis3D::DoDraw(NVGcontext *vg,
               int width, int height,
               unsigned char color[4],
               unsigned char labelColor[4],
               int overlayOffset)
{
    if ((mProjector == NULL) && (mProjectorFloat == NULL))
        return;
    
    // Manage scale: keep the same origin and scale to the second point
    BL_FLOAT dir[3] = { mP1[0] - mP0[0], mP1[1] - mP0[1], mP1[2] - mP0[2] };
    
    BL_FLOAT p1Scale[3] = { mP0[0] + dir[0]*mScale,
                            mP0[1] + dir[1]*mScale,
                            mP0[2] + dir[2]*mScale };
    
    // Project
    BL_FLOAT projP0[3];
    BL_FLOAT projP1[3];
    if (mProjector != NULL)
    {
        mProjector->ProjectPoint(projP0, mP0, width, height);
        //mProjector->ProjectPoint(projP1, mP1, width, height);
        mProjector->ProjectPoint(projP1, p1Scale, width, height);
    }
    else
    {
        if (mProjectorFloat != NULL)
        {
            float projP0f[3];
            float projP1f[3];
            
            //float p0f[3] = { mP0[0], mP0[1], mP0[2] };
            float p0f[3];
            p0f[0] = mP0[0];
            p0f[1] = mP0[1];
            p0f[2] = mP0[2];
	    
            //float p1Scalef[3] = { p1Scale[0], p1Scale[1], p1Scale[2] };
            float p1Scalef[3];
            p1Scalef[0] = p1Scale[0];
            p1Scalef[1] = p1Scale[1];
            p1Scalef[2] = p1Scale[2];
            
            mProjectorFloat->ProjectPoint(projP0f, p0f, width, height);
            mProjectorFloat->ProjectPoint(projP1f, p1Scalef, width, height);
            
            projP0[0] = projP0f[0];
            projP0[1] = projP0f[1];
            projP0[2] = projP0f[2];
            
            projP1[0] = projP1f[0];
            projP1[1] = projP1f[1];
            projP1[2] = projP1f[2];
        }
    }
    //
    nvgSave(vg);
    nvgStrokeWidth(vg, 1.0);
    
    // Axis line
    int axisColor[4] = { color[0], color[1], color[2], color[3] };
    SWAP_COLOR(axisColor);
    nvgStrokeColor(vg, nvgRGBA(axisColor[0], axisColor[1], axisColor[2], axisColor[3]));
    
    float projP0Yf = projP0[1];
    float projP1Yf = projP1[1];
#if GRAPH_CONTROL_FLIP_Y
    projP0Yf = height - projP0Yf;
    projP1Yf = height - projP1Yf;
#endif
    
    // Draw the line
    nvgBeginPath(vg);
    nvgMoveTo(vg, projP0[0], projP0Yf);
    nvgLineTo(vg, projP1[0], projP1Yf);
    nvgStroke(vg);
    
    // Labels
    for (int i = 0; i < mValues.size(); i++)
    {
        const AxisData &data = mValues[i];
        
        BL_FLOAT t = data.mT;
        const char *text = data.mText.c_str();
        
        if (mDBScale)
        {
            t = BLUtils::AmpToDBNorm(t, (BL_FLOAT)1e-15, mMinDB);
        }
        
        BL_FLOAT labelP[3];
        for (int k = 0; k < 3; k++)
        {
            labelP[k] = (1.0 - t)*mP0[k] + t* /*mP1*/ p1Scale[k];
        }
        
        BL_FLOAT projLabelP[3];
        if (mProjector != NULL)
            mProjector->ProjectPoint(projLabelP, labelP, width, height);
        else
            if (mProjectorFloat != NULL)
            {
                float projLabelPf[3];
                
                //float labelPf[3] = { labelP[0], labelP[1], labelP[2] };
                float labelPf[3];
                labelPf[0] = labelP[0];
                labelPf[1] = labelP[1];
                labelPf[2] = labelP[2];
                
                mProjectorFloat->ProjectPoint(projLabelPf, labelPf, width, height);
                
                projLabelP[0] = projLabelPf[0];
                projLabelP[1] = projLabelPf[1];
                projLabelP[2] = projLabelPf[2];
            }
        
        BL_FLOAT x = projLabelP[0];
        BL_FLOAT y = projLabelP[1];
        
#if GRAPH_CONTROL_FLIP_Y
        y = height - y;
#endif

        x += overlayOffset;
        y += overlayOffset;
        
        // Draw background text (for overlay)
        DrawText(vg, x, y,
                 FONT_SIZE, text, labelColor,
                 NVG_ALIGN_CENTER, NVG_ALIGN_MIDDLE); //NVG_ALIGN_BOTTOM);
    }
    
    nvgRestore(vg);
}

// From GraphControl10...
void
Axis3D::DrawText(NVGcontext *vg, BL_FLOAT x, BL_FLOAT y, BL_FLOAT fontSize,
                 const char *text, unsigned char color[4],
                 int halign, int valign)
{
    nvgSave(vg);
    
    nvgFontSize(vg, fontSize);
	nvgFontFace(vg, FONT);
    nvgFontBlur(vg, 0);
	nvgTextAlign(vg, halign | valign);
    
    int sColor[4] = { color[0], color[1], color[2], color[3] };
    SWAP_COLOR(sColor);
    
    nvgFillColor(vg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
	nvgText(vg, x, y, text, NULL);
    
    nvgRestore(vg);
}

#endif // IGRAPHICS_NANOVG
