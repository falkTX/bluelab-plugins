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
//  BLUpmixGraphDrawer.h
//  UST
//
//  Created by applematuer on 8/2/19.
//
//

#ifndef __UST__BLUpmixGraphDrawer__
#define __UST__BLUpmixGraphDrawer__

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>

class BLVectorscopePlug;
class GUIHelper12;

// BLUpmixGraphDrawer.h: from USTUpmixGraphDrawer.h
//
class BLUpmixGraphDrawer : public GraphCustomDrawer,
                           public GraphCustomControl
{
public:
    BLUpmixGraphDrawer(BLVectorscopePlug *plug, GraphControl12 *graph,
                       GUIHelper12 *guiHelper = NULL,
                       const char *title = NULL);
    
    virtual ~BLUpmixGraphDrawer();
    
    // GraphCustomDrawer
    virtual void PreDraw(NVGcontext *vg, int width, int height) override;
    
    // GraphCustomControl
    void OnMouseDown(float x, float y, const IMouseMod &mod) override;
    void OnMouseUp(float x, float y, const IMouseMod &mod) override;
    void OnMouseDrag(float x, float y,
                     float dX, float dY, const IMouseMod &mod) override;
    
    //
    void SetGain(BL_FLOAT gain);
    void SetPan(BL_FLOAT pan);
    void SetDepth(BL_FLOAT depth);
    void SetBrillance(BL_FLOAT brillance);
    
protected:
    void DrawSource(NVGcontext *vg, int width, int height);
    
    void ComputeSourceCenter(BL_FLOAT center[2], int width, int height);
    BL_FLOAT ComputeRad0(int height);
    BL_FLOAT ComputeRad1(int height);
    
    void SourceCenterToPanDepth(const BL_FLOAT center[2],
                                int width, int height,
                                BL_FLOAT *outPan, BL_FLOAT *outDepth);

    //
    BLVectorscopePlug *mPlug;
    GraphControl12 *mGraph;
    
    //
    BL_FLOAT mGain;
    BL_FLOAT mPan;
    BL_FLOAT mDepth;
    BL_FLOAT mBrillance;
    
    // GraphCustomControl
    int mWidth;
    int mHeight;
    
    bool mSourceIsSelected;
    bool mPrevMouseDrag;
    
    // Title
    bool mTitleSet;
    char mTitleText[256];

    // Style
    float mCircleLineWidth;
    float mLinesWidth;
    IColor mLinesColor;
    IColor mTextColor;
    int mOffsetX;
    int mTitleOffsetY;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__UST__BLUpmixGraphDrawer__) */
