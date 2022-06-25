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
//  SparseVolRender.h
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

#ifndef __BL_StereoWidth__SparseVolRender__
#define __BL_StereoWidth__SparseVolRender__

#ifdef IGRAPHICS_NANOVG

#include <vector>
#include <deque>
using namespace std;

#include <GraphControl11.h>

#define PROFILE_RENDER 0 //1

#if PROFILE_RENDER
#include <BlaTimer.h>
#endif

// NOTE: point selection doesn't work well with slice
// TODO: fix this
#define RENDER_SLICES_TEXTURE 0

class ColorMap4;
class QuadTree;

class SparseVolRender : public GraphCustomDrawer
{
public:
    class Point
    {
    public:
        BL_FLOAT mX;
        BL_FLOAT mY;
        BL_FLOAT mZ;
        
        // Intensity
        BL_FLOAT mWeight;
        
        BL_FLOAT mR;
        BL_FLOAT mG;
        BL_FLOAT mB;
        BL_FLOAT mA;
        
        // Packed color
        NVGcolor mColor;
        
        // Normalized size
        BL_FLOAT mSize;
        
        // If using a colormap
        BL_FLOAT mColorMapId;
        
        // Selection
        bool mIsSelected;
    };
    
#if RENDER_SLICES_TEXTURE
    class GPUSlice
    {
    public:
        GPUSlice();
        
        virtual ~GPUSlice();
        
        BL_FLOAT GetZ();
        
        void SetZ(BL_FLOAT z);
        
        int GetImage();
        
        void CreateImage(NVGcontext *vg, int width, int height,
                         const WDL_TypedBuf<unsigned char> &imageDataRGBA);
        
        void Draw(float corners[4][2], int width, int height, BL_FLOAT alpha);

    protected:
        NVGcontext *mVg;
        
        int mNvgImage;
        
        int mImgWidth;
        int mImgHeight;
        
        BL_FLOAT mZ;
    };
#endif
    
    SparseVolRender(int numSlices, int numPointsSlice);
    
    virtual ~SparseVolRender();
    
    void SetNumSlices(int numSlices);
    
    void SetNumPointsSlice(int numPointsSlice);
    
    // Inherited
    void PreDraw(NVGcontext *vg, int width, int height) override;
    
    void ClearSlices();
    
    void SetSlices(const vector< vector<Point> > &slices);
    
    void AddSlice(const vector<Point> &points, bool skipDisplay);

    void SetCameraAngles(BL_FLOAT angle0, BL_FLOAT angle1);
    
    // Colormap
    void SetColorMap(int colormapId);
    void SetInvertColormap(bool flag);
    
    void SetColormapRange(BL_FLOAT range);
    void SetColormapContrast(BL_FLOAT contrast);
    
    // Point size (debug)
    void SetPointSizeCoeff(BL_FLOAT coeff);
    
    // Alpha coeff (debug)
    void SetAlphaCoeff(BL_FLOAT coeff);
    
    // Selection
    void SetSelection(BL_FLOAT selection[4]);
    
    void DisableSelection();
    
    void GetPointsSelection(vector<bool> *pointFlags);
    
    //
    void SetDisplayRefreshRate(int displayRefreshRate);
    
    void SetQuality(BL_FLOAT quality);
    
    //
    static void ComputePackedPointColor(SparseVolRender::Point *p, BL_FLOAT alphaCoeff);
    
    // Reduce the number of points by associating the points that are close
    static void DecimateSlice(vector<SparseVolRender::Point> *points,
                              int maxNumPoints, BL_FLOAT alphaCoeff);
    
protected:
    // Selection
    void DrawSelection(NVGcontext *vg, int width, int height);
    
    void SelectPoints(vector<Point> *points);
    
    void UpdatePointsSelection(const vector<Point> &points);
    
    // Reduce the number of points by associating the points that are close
    //void DecimateSlice(vector<SparseVolRender::Point> *points, int maxNumPoints);
    
    // Decimate in 3D
    void DecimateVol(deque<vector<Point> > *slices);
    
    long ComputeNumPoints(const deque<vector<Point> > &slices);
    
    void VoxelToPoint(const vector<SparseVolRender::Point> &voxel,
                      SparseVolRender::Point *newPoint,
                      BL_FLOAT bboxMin[3], BL_FLOAT bboxMax[3]);

    void VoxelToPoint2(const vector<SparseVolRender::Point> &voxel,
                       SparseVolRender::Point *newPoint,
                       BL_FLOAT bboxMin[3], BL_FLOAT bboxMax[3],
                       BL_FLOAT voxelSize);
    
    void ProjectPoints(deque<vector<Point> > *slices, int width, int height);
    
    void ApplyPointsScale(deque<vector<Point> > *slices);

    
    void DecimateScreenSpace(deque<vector<Point> > *slices, int width, int height);
    
    // Compute the packed point color
    // (optimization)
    //void ComputePackedPointColor(SparseVolRender::Point *p);
    
    void ComputePackedPointColors(vector<SparseVolRender::Point> *points);

    void RefreshPackedPointColors();

    
#if RENDER_SLICES_TEXTURE
    void PointsToPixels(const vector<Point> &points,
                        WDL_TypedBuf<unsigned char> *pixels,
                        int imgSize);

    void PointsToPixelsSplat(const vector<Point> &points,
                             WDL_TypedBuf<unsigned char> *pixels,
                             int imgSize, BL_FLOAT pointSize, BL_FLOAT alpha);
    
#endif
    
    void ApplyColorMap(vector<Point> *points);

    void ApplyColorMapSlices(deque<vector<Point> > *slices);
    
    void UpdateSlicesZ();
    
    deque<vector<Point> > mSlices;
    
#if RENDER_SLICES_TEXTURE
    deque<GPUSlice> mGPUSlices;
#endif
    
    BL_FLOAT mCamAngle0;
    BL_FLOAT mCamAngle1;
    
    ColorMap4 *mColorMap;
    
    bool mInvertColormap;
    
    int mNumSlices;
    int mNumPointsSlice;
    
    // Selection
    BL_FLOAT mSelection[4];
    bool mSelectionEnabled;
    
    vector<bool> mPointsSelection;
    
    //
    int mDisplayRefreshRate;
    long mRefreshNum;
    
    // Dummy image of 1 white pixel
    // for quad rendering
    //
    // NOTE: rendering quads with or without texture: same perfs
    //
    int mWhitePixImg;
    
    // Point size coeff (debug)
    BL_FLOAT mPointSizeCoeff;
    
    // Alpha coeff
    BL_FLOAT mAlphaCoeff;
    
    // 3d quality
    BL_FLOAT mQuality;
    
    QuadTree *mQuadTree;
        
#if PROFILE_RENDER
    BlaTimer mTimer;
    long mCount;
#endif
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_StereoWidth__SparseVolRender__) */
