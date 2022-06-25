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
//  StereoVizVolRender2.h
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

#ifndef __BL_StereoWidth__StereoVizVolRender2__
#define __BL_StereoWidth__StereoVizVolRender2__

#ifdef IGRAPHICS_NANOVG

#include <vector>
#include <deque>
using namespace std;

#include <GraphControl11.h>
#include <SparseVolRender.h>

// From StereoVizVolRender
// StereoVizVolRender is used by StereoWidth
// StereoVizVolRender2 may have modifications for StereoViz
//
class StereoVizVolRender2 : public GraphCustomControl
{
public:
    StereoVizVolRender2(GraphControl11 *graphControl);
    
    virtual ~StereoVizVolRender2();
    
    virtual void AddCurveValuesWeight(const WDL_TypedBuf<BL_FLOAT> &xValues,
                                      const WDL_TypedBuf<BL_FLOAT> &yValues,
                                      const WDL_TypedBuf<BL_FLOAT> &colorWeights);
    
    // Control
    virtual void OnMouseDown(float x, float y, const IMouseMod &mod) override;
    virtual void OnMouseUp(float x, float y, const IMouseMod &mod) override;
    virtual void OnMouseDrag(float x, float y, float dX, float dY,
                             const IMouseMod &mod) override;
    virtual void /*bool*/ OnMouseDblClick(float x, float y,
                                          const IMouseMod &mod) override;
    
    // Quality parameters
    virtual void SetQualityXY(BL_FLOAT quality);
    virtual void SetQualityT(BL_FLOAT quality);
    virtual void SetQuality(BL_FLOAT quality);
    
    virtual void SetSpeed(BL_FLOAT speed);
    
    // ColorMap
    virtual void SetColormap(int colormapId);
    virtual void SetInvertColormap(bool flag);
    
    virtual void SetColormapRange(BL_FLOAT range);
    virtual void SetColormapContrast(BL_FLOAT contract);
    
    // Point size (debug)
    virtual void SetPointSizeCoeff(BL_FLOAT coeff);
    
    // Alpha coeff (debug)
    virtual void SetAlphaCoeff(BL_FLOAT coeff);
    
    // Points selection
    void GetPointsSelection(vector<bool> *pointFlags);
    
    void SetDisplayRefreshRate(int displayRefreshRate);
    
protected:
    void DistToDbScale(const WDL_TypedBuf<BL_FLOAT> &xValues,
                       const WDL_TypedBuf<BL_FLOAT> &yValues);
    
    GraphControl11 *mGraph;
    SparseVolRender *mVolRender;
    
    bool mMouseIsDown;
    int mPrevDrag[2];
  
    bool mIsSelecting;
    int mSelection[4];
    bool mSelectionActive;
    
    BL_FLOAT mAngle0;
    BL_FLOAT mAngle1;
    
    long int mAddNum;
    
    // Quality
    //int mNumPointsSlice;
    int mSpeed;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_StereoWidth__StereoVizVolRender2__) */
