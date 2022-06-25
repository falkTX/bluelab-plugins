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
//  StereoVizVolRender3.h
//  BL-Waves
//
//  Created by applematuer on 10/13/18.
//
//

#ifndef __BL_Waves__WavesRender__
#define __BL_Waves__WavesRender__

#ifdef IGRAPHICS_NANOVG

#include <vector>
#include <deque>
using namespace std;

#include <GraphControl12.h>

//#include <LinesRender.h>
#include <LinesRender2.h>

#include <View3DPluginInterface.h>

// Sides
//#define MAX_ANGLE_0 70.0

// Set to 90 for debugging
#define MAX_CAM_ANGLE_0 90.0

// Above
#define MAX_CAM_ANGLE_1 90.0 //70.0

// Below
//#define MIN_ANGLE_1 -20.0

// Almost horizontal (a little above)
#define MIN_CAM_ANGLE_1 15.0


//class Wav3s;
class Axis3D;

class WavesRender : public GraphCustomControl
{
public:
    WavesRender(View3DPluginInterface *plug,
                GraphControl12 *graphControl,
                int bufferSize, BL_FLOAT sampleRate);
    
    virtual ~WavesRender();
    
    void SetGraph(GraphControl12 *graphControl);
    
    void Reset(BL_FLOAT sampleRate);
    
    virtual void AddMagns(const WDL_TypedBuf<BL_FLOAT> &magns);
    
    // Control
    virtual void OnMouseDown(float x, float y, const IMouseMod &mod) override;
    virtual void OnMouseUp(float x, float y, const IMouseMod &mod) override;
    virtual void OnMouseDrag(float x, float y, float dX, float dY,
                             const IMouseMod &mod) override;
    virtual void /*bool*/ OnMouseDblClick(float x, float y,
                                          const IMouseMod &mod) override;
    virtual void OnMouseWheel(float x, float y,
                              const IMouseMod &mod,
                              float d) override;
    
    virtual bool OnKeyDown(float x, float y, const IKeyPress& key) override;
    
    // Parameters
    virtual void SetMode(LinesRender2::Mode mode);
    virtual void SetSpeed(BL_FLOAT speed);
    virtual void SetDensity(BL_FLOAT density);
    virtual void SetScale(BL_FLOAT scale);
    virtual void SetScrollDirection(LinesRender2::ScrollDirection dir);
    virtual void SetShowAxes(bool flag);
    virtual void SetDBScale(bool flag, BL_FLOAT minDB);

    
    // for parameter sent from plug (save state and automation)
    void SetCamAngle0(BL_FLOAT angle);
    void SetCamAngle1(BL_FLOAT angle);
    void SetCamFov(BL_FLOAT angle);
    
    // Force refresh
    void SetDirty(bool pushParamToPlug);
    
    void SetColors(unsigned char color0[4], unsigned char color1[4]);
    
protected:
    void MagnsToPoints(vector<LinesRender2::Point> *points,
                       const WDL_TypedBuf<BL_FLOAT> &magns);

    BL_FLOAT FreqToMelNorm(BL_FLOAT normFreq);
    
    void TransformORXX(WDL_TypedBuf<BL_FLOAT> *magns);

    // Axis
    void CreateFreqsAxis();
    void UpdateAmpsAxis(bool dbScale);
    
    //
    View3DPluginInterface *mPlug;
    
    GraphControl12 *mGraph;
    LinesRender2 *mLinesRender;
    
    Axis3D *mFreqsAxis;
    
    Axis3D *mAmpsAxis;
    
    // Selection
    bool mMouseIsDown;
    int mPrevDrag[2];
    
    // Used to detect pure mouse up, without drag
    bool mPrevMouseDrag;
    
    // Rotation
    BL_FLOAT mCamAngle0;
    BL_FLOAT mCamAngle1;
    
    BL_FLOAT mScale;
    
    int mBufferSize;
    BL_FLOAT mSampleRate;
    
    //
    unsigned long long int mAddNum;
    
    // Easter Egg
    bool mORXXMode;
    int mORXXKeyGuessStep;

private:
    // Tmp buffers
    vector<LinesRender2::Point> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf5;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_Waves__WavesRender__) */
