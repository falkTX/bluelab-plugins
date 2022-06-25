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
//  PanogramCustomDrawer.h
//  BL-Panogram
//
//  Created by applematuer on 10/21/19.
//
//

#ifndef __BL_Panogram__PanogramCustomDrawer__
#define __BL_Panogram__PanogramCustomDrawer__

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>

class PanogramCustomDrawer : public GraphCustomDrawer
{
public:
    enum ViewOrientation
    {
        HORIZONTAL = 0,
        VERTICAL
    };
    
    struct State
    {
        bool mBarActive;
        BL_FLOAT mBarPos;
        
        bool mSelectionActive;
        BL_FLOAT mSelection[4];
        
        bool mPlayBarActive;
        BL_FLOAT mPlayBarPos;

        ViewOrientation mViewOrientation;
    };
    
    PanogramCustomDrawer(Plugin *plug,
                         BL_FLOAT x0, BL_FLOAT y0, BL_FLOAT x1, BL_FLOAT y1,
                         State *state);
    
    virtual ~PanogramCustomDrawer() {}
    
    State *GetState();
    
    void Reset();
    
    // The graph will destroy it automatically
    bool IsOwnedByGraph() override { return true; }

    bool NeedRedraw() override;
    
    // Implement one of the two methods, depending on when
    // you whant to draw
    
    // Draw before everything
    void PreDraw(NVGcontext *vg, int width, int height) override {}
    
    // Draw after everything
    void PostDraw(NVGcontext *vg, int width, int height) override;
    
    //
    void ClearBar();
    
    void ClearSelection();
    
    void SetBarPos(BL_FLOAT pos);
    BL_FLOAT GetBarPos();
    
    void SetBarActive(bool flag);
    bool IsBarActive();
    
    void SetSelection(BL_FLOAT x0, BL_FLOAT y0,
                      BL_FLOAT x1, BL_FLOAT y1);
    void SetSelectionActive(bool flag);
    
    void GetSelection(BL_FLOAT *x0, BL_FLOAT *y0,
                      BL_FLOAT *x1, BL_FLOAT *y1);
    
    bool IsSelectionActive();
    
    BL_FLOAT GetPlayBarPos();
    void SetPlayBarPos(BL_FLOAT pos, bool activate);
    
    bool IsPlayBarActive();
    void SetPlayBarActive(bool flag);
    
    // Normalized inside selection
    void SetSelPlayBarPos(BL_FLOAT pos);

    void SetViewOrientation(ViewOrientation orientation);
    
protected:
    void DrawBar(NVGcontext *vg, int width, int height);
    void DrawSelection(NVGcontext *vg, int width, int height);
    void DrawPlayBar(NVGcontext *vg, int width, int height);
    
    //
    Plugin *mPlug;
    
    State *mState;
    
    BL_FLOAT mBounds[4];

    bool mNeedRedraw;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_Panogram__PanogramCustomDrawer__) */
