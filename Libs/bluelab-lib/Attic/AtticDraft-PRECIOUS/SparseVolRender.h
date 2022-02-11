//
//  SparseVolRender.h
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

#ifndef __BL_StereoWidth__SparseVolRender__
#define __BL_StereoWidth__SparseVolRender__

#include <vector>
#include <deque>
using namespace std;

#include "nanovg.h"

#include <GraphControl10.h>

class ColorMap4;

class SparseVolRender : public GraphCustomDrawer
{
public:
    class Point
    {
    public:
        double mX;
        double mY;
        double mZ;
        
        // Intensity
        double mWeight;
        
        double mR;
        double mG;
        double mB;
        double mA;
        
        // Packed color
        NVGcolor mColor;
        
        // Normalized size
        double mSize;
        
        // If using a colormap
        double mColorMapId;
    };
    
    SparseVolRender(int numSlices, int numPointsSlice);
    
    virtual ~SparseVolRender();
    
    void SetNumSlices(int numSlices);
    
    void SetNumPointsSlice(int numPointsSlice);
    
    // Inherited
    void PreDraw(NVGcontext *vg, int width, int height);
    
    void ClearSlices();
    
    void SetSlices(const vector< vector<Point> > &slices);
    
    void AddSlice(const vector<Point> &points);

    void SetCameraAngles(double angle0, double angle1);
    
    void SetColorMap(int colormapId);
    
    void SetInvertColormap(bool flag);
    
protected:
    // Reduce the number of points by associating the points that are close
    static void DecimateSlice(vector<SparseVolRender::Point> *points, int maxNumPoints);
    
    // Compute the packed point color
    // (optimization)
    static void ComputePackedPointColor(SparseVolRender::Point *p);
    
    static void ComputePackedPointColors(vector<SparseVolRender::Point> *points);
    
    void ApplyColorMap(vector<Point> *points);

    void ApplyColorMapSlices(deque<vector<Point> > *slices);
    
    void UpdateSlicesZ();
    
    deque<vector<Point> > mSlices;
    
    double mCamAngle0;
    double mCamAngle1;
    
    ColorMap4 *mColorMap;
    
    bool mInvertColormap;
    
    int mNumSlices;
    int mNumPointsSlice;
};

#endif /* defined(__BL_StereoWidth__SparseVolRender__) */
