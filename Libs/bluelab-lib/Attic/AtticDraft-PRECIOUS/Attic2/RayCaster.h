//
//  RayCaster.h
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

#ifndef __BL_StereoWidth__RayCaster__
#define __BL_StereoWidth__RayCaster__

#include <vector>
#include <deque>
using namespace std;

#include "nanovg.h"

#include <GraphControl10.h>

#define PROFILE_RENDER 0

#if PROFILE_RENDER
#include <BlaTimer.h>
#endif

class ColorMap4;

class RayCaster : public GraphCustomDrawer
{
public:
    class Point
    {
    public:
        double mX;
        double mY;
        double mZ;
        
        double mSize;
        
        double mWeight;
        
        unsigned char mR;
        unsigned char mG;
        unsigned char mB;
        unsigned char mA;
        
        bool mIsSelected;
        
        static bool IsSmaller(const Point &p0, const Point &p1)
        {
            return p0.mZ < p1.mZ;
        }
    };
    
    RayCaster(int numSlices, int numPointsSlice);
    
    virtual ~RayCaster();
    
    void SetNumSlices(long numSlices);
    
    long GetNumSlices();
    
    // Inherited
    void PreDraw(NVGcontext *vg, int width, int height);
    
    void ClearSlices();
    
    void SetSlices(const vector< vector<Point> > &slices);
    
    void AddSlice(const vector<Point> &points, bool skipDisplay);

    void SetCameraAngles(double angle0, double angle1);
    
    // Colormap
    void SetColorMap(int colormapId);
    void SetInvertColormap(bool flag);
    
    void SetColormapRange(double range);
    void SetColormapContrast(double contrast);
    
    // Selection
    void SetSelection(const double selection[4]);

    void GetSelection(double selection[4]);
    
    void EnableSelection();
    
    void DisableSelection();
    
    bool SelectionEnabled();
    
    bool GetCenterSliceSelection(vector<bool> *selection,
                                 vector<double> *xCoords,
                                 long *sliceNum);
    
    // Extract screen selection from volume selection
    void VolumeToSelection();
    
    //
    void SetDisplayRefreshRate(int displayRefreshRate);
    
    void SetQuality(double quality);
    
    void SetPointScale(double scale);
    
    void SetAlphaScale(double scale);
    
protected:
    // Selection
    void DrawSelection(NVGcontext *vg, int width, int height);
   
    void DBG_DrawSelectionVolume(NVGcontext *vg, int width, int height);
    
    void SelectPoints(vector<Point> *projPoints, int width, int height);
    
    void PropagateSelection(const vector<Point> &projPoints);
    
    void ProjectPoints(vector<Point> *slices, int width, int height);
    
    void RayCast(vector<Point> *slices, int width, int height);
    
    void DoRayCast(vector<Point> *slices, int width, int height);
    
    void DecimateScreenSpace(deque<vector<Point> > *slices, int width, int height);
    
    void ComputePackedPointColor(const RayCaster::Point &p, NVGcolor *color);
    
    void ApplyColorMap(vector<Point> *points);
    
    void UpdateSlicesZ();
    
    static void DequeToVec(vector<RayCaster::Point> *res, const deque<vector<RayCaster::Point> > &que);
    
    int GetResolution();
    
    void ProjectPoint(double projP[3], const double p[3], int width, int height);
    
    long GetCenterSlice();
    
    //
    void SelectionToVolume();

    void ScreenToAlignedVolume(const double screenSelectionNorm[4],
                               double alignedVolumeSelection[4]);
    
    void AlignedVolumeToScreen(const double alignedVolumeSelection[4],
                               double screenSelectionNorm[4]);
    
    //void ProjectFace(double projFace[2][3]);
    
    void ReorderScreenSelection(double screenSelection[4]);
    
    // DEBUG
    void DBG_SelectSlice(long sliceNum);
    
    bool SelectionIntersectSlice2D(const double selection2D[2][2],
                                   const double projSlice2D[4][2]);
    
    typedef struct SelectionPoint
    {
        double mX;
        double mY;
        double mZ;
        
        SelectionPoint()
        {
            mX = 0.0;
            mY = 0.0;
            mZ = 0.0;
        }
        
        SelectionPoint(double x, double y, double z)
        {
            mX = x;
            mY = y;
            mZ = z;
        }
    } SelectionPoint;
    //
    
    deque<vector<Point> > mSlices;
    
    //
    
    double mCamAngle0;
    double mCamAngle1;
    
    ColorMap4 *mColorMap;
    
    bool mInvertColormap;
    
    long mNumSlices;
    
    // Selection
    
    // 2D selection, for screen
    double mScreenSelection[4];
    
    bool mSelectionEnabled;
    
    // 3D selection, for volume
    // (xmin, xmax), (ymin, ymax), (zmin, zmax)
    SelectionPoint mVolumeSelection[8];
    
    bool mVolumeSelectionEnabled;
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
    
    double mQuality;
    
    double mPointScale;
    
    double mAlphaScale;
    
    // Optim: GOOD (optim x2)
    // Do not re-allocate points each time
    // Keep a buffer
    vector<RayCaster::Point> mTmpPoints;
    
    // Optim: GOOD (optim x2)
    // Do not re-allocate the grid each time
    vector<vector<vector<Point> > > mTmpGrid;
    
    int mViewWidth;
    int mViewHeight;
    
#if PROFILE_RENDER
    BlaTimer mTimer0;
    long mCount0;
#endif
};

#endif /* defined(__BL_StereoWidth__RayCaster__) */
