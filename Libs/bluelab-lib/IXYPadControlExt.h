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
 
#ifndef IXY_PAD_CONTROL_EXT_H
#define IXY_PAD_CONTROL_EXT_H

#include <vector>
using namespace std;

#include <IControl.h>

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;
using namespace iplug::igraphics;

class IXYPadControlExtListener
{
public:
    virtual void OnHandleChanged(int handleNum)  = 0;
};

// Manage multiple handles
class IXYPadControlExt : public IControl
{
 public:
    enum HandleState
    {
        HANDLE_NORMAL = 1,
        HANDLE_HIGHLIGHTED,
        HANDLE_GRAYED_OUT
    };
    
    IXYPadControlExt(Plugin *plug,
                     const IRECT& bounds,
                     const std::initializer_list<int>& params,
                     const IBitmap& trackBitmap,
                     float borderSize = 0.0,
                     bool reverseY = false);

    virtual ~IXYPadControlExt();

    void SetListener(IXYPadControlExtListener *listener);
    
    void AddHandle(IGraphics *pGraphics, const char *handleBitmapFname,
                   const std::initializer_list<int>& params);
    int GetNumHandles();

    void SetHandleEnabled(int handleNum, bool flag);
    bool IsHandleEnabled(int handleNum);

    void SetHandleState(int handleNum, HandleState state);
    
    void Draw(IGraphics& g) override;

    void OnMouseDown(float x, float y, const IMouseMod& mod) override;
    void OnMouseUp(float x, float y, const IMouseMod& mod) override;
    void OnMouseDrag(float x, float y, float dX, float dY,
                     const IMouseMod& mod) override;

    // Handle normalized position
    bool GetHandleNormPos(int handleNum,
                          float *tx, float *ty,
                          bool normRectify = false) const;

    void RefreshAllHandlesParams();
    
 protected:
    void DrawTrack(IGraphics& g);
    void DrawHandles(IGraphics& g);

    // Ensure that the handle doesn't go out of the track at all 
    void PixelsToParams(int handleNum, float *x, float *y);
    void ParamsToPixels(int handleNum, float *x, float *y);

    // Return the handle number, or -1 if no handle
    int MouseOnHandle(float x, float y,
                      float *offsetX, float *offsetY);
        
    //
    IBitmap mTrackBitmap;
    IBitmap mHandleBitmap;

    // Border size, or "stroke width"
    float mBorderSize;

    bool mReverseY;
    
    bool mMouseDown;

    //
    Plugin *mPlug;

    //
    struct Handle
    {
        IBitmap mBitmap;
        int mParamIdx[2];

        float mOffsetX;
        float mOffsetY;
        
        float mPrevX;
        float mPrevY;

        bool mIsGrabbed;

        bool mIsEnabled;

        HandleState mState;
    };

    vector<Handle> mHandles;

    //
    IXYPadControlExtListener *mListener;
};

#endif
