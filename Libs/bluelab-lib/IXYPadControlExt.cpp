#include <GUIHelper12.h>

#include "IXYPadControlExt.h"

IXYPadControlExt::IXYPadControlExt(Plugin *plug,
                                   const IRECT& bounds,
                                   const std::initializer_list<int>& params,
                                   const IBitmap& trackBitmap,
                                   float borderSize, bool reverseY)
: IControl(bounds, params)
{
    mPlug = plug;
    
    mTrackBitmap = trackBitmap;

    mBorderSize = borderSize;

    mReverseY = reverseY;
    
    mMouseDown = false;

    mListener = NULL;
}

IXYPadControlExt::~IXYPadControlExt() {}

void
IXYPadControlExt::SetListener(IXYPadControlExtListener *listener)
{
    mListener = listener;
}

void
IXYPadControlExt::AddHandle(IGraphics *pGraphics, const char *handleBitmapFname,
                            const std::initializer_list<int>& params)
{
    if (params.size() != 2)
        return;

    vector<int> paramsVec;
    for (auto& paramIdx : params) {
        paramsVec.push_back(paramIdx);
    }
    
    IBitmap handleBitmap = pGraphics->LoadBitmap(handleBitmapFname, 1);
    
    Handle handle;
    handle.mBitmap = handleBitmap;
    handle.mParamIdx[0] = paramsVec[0];
    handle.mParamIdx[1] = paramsVec[1];

    handle.mOffsetX = 0.0;
    handle.mOffsetY = 0.0;

    handle.mPrevX = 0.0;
    handle.mPrevY = 0.0;

    handle.mIsGrabbed = false;

    handle.mIsEnabled = true;
    
    mHandles.push_back(handle);
}

int
IXYPadControlExt::GetNumHandles()
{
    return mHandles.size();
}

void
IXYPadControlExt::SetHandleEnabled(int handleNum, bool flag)
{
    if (handleNum >= mHandles.size())
        return;

    mHandles[handleNum].mIsEnabled = flag;

    SetDirty(false);
}

bool
IXYPadControlExt::IsHandleEnabled(int handleNum)
{
    if (handleNum >= mHandles.size())
        return false;

    return mHandles[handleNum].mIsEnabled;
}

void
IXYPadControlExt::Draw(IGraphics& g)
{
    DrawTrack(g);
    DrawHandles(g);
}

void
IXYPadControlExt::OnMouseDown(float x, float y, const IMouseMod& mod)
{
#ifndef __linux__
  if (mod.A)
  {
      //SetValueToDefault(GetValIdxForPos(x, y));
      if (!mHandles.empty())
      {
          if (mHandles[0].mIsEnabled)
          {
              // Set values to defaults
              IParam *param0 = mPlug->GetParam(mHandles[0].mParamIdx[0]);
              param0->Set(param0->GetDefault(true));
              
              IParam *param1 = mPlug->GetParam(mHandles[0].mParamIdx[1]);
              param1->Set(param1->GetDefault(true));
              
              // Force refresh, in case of handle param is also used e.g on knobs
              GUIHelper12::RefreshParameter(mPlug, mHandles[0].mParamIdx[0]);
              GUIHelper12::RefreshParameter(mPlug, mHandles[0].mParamIdx[1]);
              
              if (mListener != NULL)
                  mListener->OnHandleChanged(0);
          }
      }
      
      return;
  }
#else
  // On Linux, Alt+click does nothing (at least on my xubuntu)
  // So use Ctrl-click instead to reset parameters
  if (mod.C)
  {
      //SetValueToDefault(GetValIdxForPos(x, y));
      if (!mHandles.empty())
      {
          if (mHandles[0].mIsEnabled)
          {
              // Set values to defaults
              IParam *param0 = mPlug->GetParam(mHandles[0].mParamIdx[0]);
              param0->Set(param0->GetDefault(true));
              
              IParam *param1 = mPlug->GetParam(mHandles[0].mParamIdx[1]);
              param1->Set(param1->GetDefault(true));
              
              // Force refresh, in case of handle param is also used e.g on knobs
              GUIHelper12::RefreshParameter(mPlug, mHandles[0].mParamIdx[0]);
              GUIHelper12::RefreshParameter(mPlug, mHandles[0].mParamIdx[1]);
              
              if (mListener != NULL)
                  mListener->OnHandleChanged(0);
          }
      }
      
      return;
  }
#endif
        
    mMouseDown = true;

    for (int i = 0; i < mHandles.size(); i++)
    {
        Handle &handle = mHandles[i];
        if (!handle.mIsEnabled)
            continue;
        
        handle.mPrevX = x;
        handle.mPrevY = y;
    }
    
    // Check if we clicked exactly on and handle
    // In this case, do not make the handle jump
    float offsetX;
    float offsetY;
    int handleNum = MouseOnHandle(x, y, &offsetX, &offsetY);
    if (handleNum != -1)
    {
        mHandles[handleNum].mOffsetX = offsetX;
        mHandles[handleNum].mOffsetY = offsetY;

        mHandles[handleNum].mIsGrabbed = true;
    }
    
    // Direct jump
    OnMouseDrag(x, y, 0., 0., mod);
}

void
IXYPadControlExt::OnMouseUp(float x, float y, const IMouseMod& mod)
{
    mMouseDown = false;

    for (int i = 0; i < mHandles.size(); i++)
    {
        Handle &handle = mHandles[i];

        if (!handle.mIsEnabled)
            continue;
        
        handle.mOffsetX = 0.0;
        handle.mOffsetY = 0.0;

        handle.mIsGrabbed = false;
    }
    
    SetDirty(true);
}

void
IXYPadControlExt::OnMouseDrag(float x, float y, float dX, float dY,
                              const IMouseMod& mod)
{
    for (int i = 0; i < mHandles.size(); i++)
    {
        Handle &handle = mHandles[i];

        if (!handle.mIsEnabled)
            continue;
        
        if (!handle.mIsGrabbed)
            continue;
        
        // For sensitivity
        if (mod.S) // Shift pressed => move slower
        {
#define PRECISION 0.25
            
            float newX = handle.mPrevX + PRECISION*dX;
            float newY = handle.mPrevY + PRECISION*dY;

            handle.mPrevX = newX;
            handle.mPrevY = newY;
            
            x = newX;
            y = newY;
        }

        // Manage well when hitting/releasing shift witing the same drag action
        handle.mPrevX = x;
        handle.mPrevY = y;
        
        // For dragging from click on the handle
        x -= handle.mOffsetX;
        y -= handle.mOffsetY;
    
        // Original code
        mRECT.Constrain(x, y);
    
        float xn = x;
        float yn = y;
        PixelsToParams(&xn, &yn);

        mPlug->GetParam(handle.mParamIdx[0])->SetNormalized(xn);
        mPlug->GetParam(handle.mParamIdx[1])->SetNormalized(yn);

        // Force refresh, in case of handle param is also used e.g on knobs
        GUIHelper12::RefreshParameter(mPlug, handle.mParamIdx[0]);
        GUIHelper12::RefreshParameter(mPlug, handle.mParamIdx[1]);

        if (mListener != NULL)
            mListener->OnHandleChanged(i);
    }
    
    SetDirty(true);
}
  
void
IXYPadControlExt::DrawTrack(IGraphics& g)
{
    IBlend blend = GetBlend();
    g.DrawBitmap(mTrackBitmap,
                 GetRECT().GetCentredInside(IRECT(0, 0, mTrackBitmap)),
                 0, &blend);
}

void
IXYPadControlExt::DrawHandles(IGraphics& g)
{
    // Draw them in reverse order, so we grab what we see
    for (int i = mHandles.size() - 1; i >= 0; i--)
    {
        const Handle &handle = mHandles[i];

        if (!handle.mIsEnabled)
            continue;
        
        float val0 = mPlug->GetParam(handle.mParamIdx[0])->Value();
        float val1 = mPlug->GetParam(handle.mParamIdx[1])->Value();
        
        float xn = mPlug->GetParam(handle.mParamIdx[0])->ToNormalized(val0);
        float yn = mPlug->GetParam(handle.mParamIdx[1])->ToNormalized(val1);

        if (!mReverseY)
            yn = 1.0 - yn;
    
        float x = xn;
        float y = yn;
        ParamsToPixels(&x, &y);

        int w = handle.mBitmap.W();
        int h = handle.mBitmap.H();

        IRECT handleRect(x, y, x + w, y + h);
        
        IBlend blend = GetBlend();
        g.DrawBitmap(handle.mBitmap, handleRect, 0, &blend);
    }
}

void
IXYPadControlExt::PixelsToParams(float *x, float *y)
{
    if (mHandles.empty())
        return;

    if (!mHandles[0].mIsEnabled)
        return;
    
    float w = mHandles[0].mBitmap.W();
    float h = mHandles[0].mBitmap.H();
    
    *x = (*x - (mRECT.L + w/2 + mBorderSize)) /
        (mRECT.W() - w - mBorderSize*2.0);
    *y = 1.f - ((*y - (mRECT.T + h/2 + mBorderSize)) /
                (mRECT.H() - h - mBorderSize*2.0));

    if (mReverseY)
        *y = 1.0 - *y;
    
    // Bounds
    if (*x < 0.0)
        *x = 0.0;
    if (*x > 1.0)
        *x = 1.0;

    if (*y < 0.0)
        *y = 0.0;
    if (*y > 1.0)
        *y = 1.0;
}

void
IXYPadControlExt::ParamsToPixels(float *x, float *y)
{
    if (mHandles.empty())
        return;

    if (!mHandles[0].mIsEnabled)
        return;
    
    float w = mHandles[0].mBitmap.W();
    float h = mHandles[0].mBitmap.H();
    
    *x = mRECT.L + mBorderSize + (*x)*(mRECT.W() - w - mBorderSize*2.0);
    *y = mRECT.T + mBorderSize + (*y)*(mRECT.H() - h - mBorderSize*2.0);
}

// Used to avoid handle jumps when clicking directly on it
// (In this case, we want to drag smoothly from the current position
// to a nearby position, without any jump)
int
IXYPadControlExt::MouseOnHandle(float mx, float my,
                                float *offsetX, float *offsetY)
{
    for (int i = 0; i < mHandles.size(); i++)
    {
        Handle &handle = mHandles[i];

        if (!handle.mIsEnabled)
            continue;
        
        float val0 = mPlug->GetParam(handle.mParamIdx[0])->Value();
        float val1 = mPlug->GetParam(handle.mParamIdx[1])->Value();
        
        float xn = mPlug->GetParam(handle.mParamIdx[0])->ToNormalized(val0);
        float yn = mPlug->GetParam(handle.mParamIdx[1])->ToNormalized(val1);
            
        if (!mReverseY)
            yn = 1.0 - yn;
    
        float x = xn;
        float y = yn;
        ParamsToPixels(&x, &y);

        int w = handle.mBitmap.W();
        int h = handle.mBitmap.H();

        IRECT handleRect(x, y, x + w, y + h);
        
        bool onHandle = handleRect.Contains(mx, my);

        *offsetX = 0.0;
        *offsetY = 0.0;
        if (onHandle)
        {
            *offsetX = mx - (x + w/2);
            *offsetY = my - (y + h/2);
        }

        if (onHandle)
            return i;
    }

    return -1;
}
