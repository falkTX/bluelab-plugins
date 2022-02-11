//
//  RayCaster.cpp
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

#include <algorithm>
using namespace std;

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <glm/gtx/intersect.hpp>

#include <ColorMap4.h>

#include <Utils.h>

#include "RayCaster.h"

#define DISABLE_DISPLAY_REFRESH_RATE 1

#define TRANSPARENT_COLORMAP 1

#define RESOLUTION_0 32
#define RESOLUTION_1 512

#define SELECT_GRAY_COLOR_COEFF 0.5

#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

static void
CameraModelProjMat(int winWidth, int winHeight, float angle0, float angle1,
                   glm::mat4 *model, glm::mat4 *proj)
{
    glm::vec3 pos(0.0f, 0.0, -2.0f);
    glm::vec3 target(0.0f, 0.37f, 0.0f);
    
    glm::vec3 up(0.0, 1.0f, 0.0f);
    
    glm::vec3 lookVec(target.x - pos.x, target.y - pos.y, target.z - pos.z);
    float radius = glm::length(lookVec);
    
    angle1 *= 3.14/180.0;
    float newZ = cos(angle1)*radius;
    float newY = sin(angle1)*radius;
    
    // Seems to work (no need to touch up vector)
    pos.z = newZ;
    pos.y = newY;
    
    glm::mat4 viewMat = glm::lookAt(pos, target, up);
    
    glm::mat4 perspMat = glm::perspective(45.0f/*60.0f*/, ((float)winWidth)/winHeight, NEAR_PLANE, FAR_PLANE);
    
    glm::mat4 modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(1.3f, 1.3f, 1.3f));
    
    modelMat = glm::translate(modelMat, glm::vec3(0.0f, -0.5f, 0.0f)); //
    modelMat = glm::rotate(modelMat, angle0, glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = glm::translate(modelMat, glm::vec3(0.0f, 0.33/*0.0f*/, 0.0f));
    
    *model = viewMat * modelMat;
    *proj = perspMat;
}

static void
CameraUnProject(int winWidth, int winHeight, float angle0, float angle1,
                const double p2[3], double p3[3])
{
    glm::mat4 model;
    glm::mat4 proj;
    CameraModelProjMat(winWidth, winHeight, angle0, angle1,
                       &model, &proj);
    
    glm::vec3 win(p2[0], p2[1], p2[2]);
    
    glm::vec4 viewport(0, 0, winWidth, winHeight);
    glm::vec3 world = glm::unProject(win, model, proj, viewport);
    
    p3[0] = world.x;
    p3[1] = world.y;
    p3[2] = world.z;
}

// See: https://gamedev.stackexchange.com/questions/142918/intersecting-triangle-and-boundingbox-in-3d-environment
//

//////////////////////////////////////////////////////
//    use separating axis theorem to test overlap
//    between triangle and box
//    need to test for overlap in these directions:
//    1) the {x,y,z}-directions (actually, since we use
//       the AABB of the triangle
//       we do not even need to test these)
//    2) normal of the triangle
//    3) crossproduct(edge from tri, {x,y,z}-directin)
//       this gives 3x3=9 more tests
#define OUTSIDE   -1
#define INTERSECT  0
#define INSIDE     1
static int
TriangleVsAABB2(const glm::vec3 &bmin,
                const glm::vec3 &bmax,
                const glm::vec3 &boxcenter,
                const glm::vec3 &boxhalfsize,
                const glm::vec3 &trivert0,
                const glm::vec3 &trivert1,
                const glm::vec3 &trivert2,
                const glm::vec3 &normal)
{
    
    // if triangle is entirely inside the bounding box, then exit
    
    if ((trivert0.x >= bmin.x && trivert0.x <= bmax.x) &&
        (trivert0.y >= bmin.y && trivert0.y <= bmax.y) &&
        (trivert0.z >= bmin.z && trivert0.z <= bmax.z))
        if ((trivert1.x >= bmin.x && trivert1.x <= bmax.x) &&
            (trivert1.y >= bmin.y && trivert1.y <= bmax.y) &&
            (trivert1.z >= bmin.z && trivert1.z <= bmax.z))
            if ((trivert2.x >= bmin.x && trivert2.x <= bmax.x) &&
                (trivert2.y >= bmin.y && trivert2.y <= bmax.y) &&
                (trivert2.z >= bmin.z && trivert2.z <= bmax.z))
                return INSIDE;
    
    // if we get here triangle maybe intersecting the cube or could be totally out
    
    float d,min,max;
    float p0, p1, p2;
    glm::vec3 fe,vmin,vmax;
    glm::vec3 v0,v1,v2;
    glm::vec3 e0, e1, e2;
    
    // center at 0,0,0
    
    v0 = trivert0 - boxcenter;
    v1 = trivert1 - boxcenter;
    v2 = trivert2 - boxcenter;
    
    // compute triangle edges
    
    e0 = v1 - v0;
    e1 = v2 - v1;
    e2 = v0 - v2;
    
    //  test the 9 tests for axis overlapping
    
    // axis test x01
    
    fe = glm::abs(e0);
    
    p0 = e0.z * v0.y - e0.y * v0.z;
    p2 = e0.z * v2.y - e0.y * v2.z;
    
    min = p2;
    max = p0;
    
    if (p0<p2)
    {
        min = p0;
        max = p2;
    }
    
    d = fe.z * boxhalfsize.y + fe.y * boxhalfsize.z;
    
    if (min>d || max<-d) return OUTSIDE;
    
    // axis test y02
    
    p0 = -e0.z * v0.x + e0.x * v0.z;
    p2 = -e0.z * v2.x + e0.x * v2.z;
    
    min = p2;
    max = p0;
    
    if (p0<p2) { min = p0; max = p2; }
    
    d = fe.z * boxhalfsize.x + fe.x * boxhalfsize.z;
    
    if (min>d || max<-d) return OUTSIDE;
    
    // axis test z12
    
    p1 = e0.y * v1.x - e0.x * v1.y;
    p2 = e0.y * v2.x - e0.x * v2.y;
    
    min = p1;
    max = p2;
    
    if (p2<p1)
    {
        min = p2;
        max = p1;
    }
    
    d = fe.y * boxhalfsize.x + fe.x * boxhalfsize.y;
    
    if (min>d || max<-d) return OUTSIDE;
    
    fe = glm::abs(e1);
    
    // axis test x01
    
    p0 = e1.z * v0.y - e1.y * v0.z;
    p2 = e1.z * v2.y - e1.y * v2.z;
    
    min = p2;
    max = p0;
    
    if (p0<p2)
    {
        min = p0;
        max = p2;
    }
    
    d = fe.z * boxhalfsize.y + fe.y * boxhalfsize.z;
    
    if (min>d || max<-d) return OUTSIDE;
    
    // axis test y02
    
    p0 = -e1.z * v0.x + e1.x * v0.z;
    p2 = -e1.z * v2.x + e1.x * v2.z;
    
    min = p2;
    max = p0;
    
    if (p0<p2)
    {
        min = p0;
        max = p2;
    }
    
    d = fe.z * boxhalfsize.x + fe.x * boxhalfsize.z;
    
    if (min>d || max<-d) return OUTSIDE;
    
    // axis test z0
    
    p0 = e1.y * v0.x - e1.x * v0.y;
    p1 = e1.y * v1.x - e1.x * v1.y;
    
    min = p1;
    max = p0;
    
    if (p0<p1)
    {
        min = p0;
        max = p1;
    }
    
    d = fe.y* boxhalfsize.x + fe.x * boxhalfsize.y;
    
    if (min>d || max<-d) return OUTSIDE;
    
    fe = glm::abs(e2);
    
    // axis test x02
    
    p0 = e2.z * v0.y - e2.y * v0.z;
    p1 = e2.z * v1.y - e2.y * v1.z;
    
    min = p1;
    max = p0;
    
    if (p0<p1)
    {
        min = p0;
        max = p1;
    }
    
    d = fe.z * boxhalfsize.y + fe.y * boxhalfsize.z;
    
    if (min>d || max<-d) return OUTSIDE;
    
    // axis test y01
    
    p0 = -e2.z * v0.x + e2.x * v0.z;
    p1 = -e2.z * v1.x + e2.x * v1.z;
    
    min = p1;
    max = p0;
    
    if (p0<p1)
    {
        min = p0;
        max = p1;
    }
    
    d = fe.z * boxhalfsize.x + fe.x * boxhalfsize.z;
    
    if (min>d || max<-d) return OUTSIDE;
    
    // axis test z12
    
    p1 = e2.y * v1.x - e2.x * v1.y;
    p2 = e2.y * v2.x - e2.x * v2.y;
    
    min = p1;
    max = p2;
    
    if (p2<p1)
    {
        min = p2;
        max = p1;
    }
    
    d = fe.y * boxhalfsize.x + fe.x * boxhalfsize.y;
    
    if (min>d || max<-d) return 0;
    
    //  first test overlap in the {x,y,z}-directions
    //  find min, max of the triangle each direction,
    //  and test for overlap in
    //  that direction -- this is equivalent to testing
    //  a minimal AABB around
    //  the triangle against the AABB
    
    // test in X-direction
    
    min = v0.x;
    max = v0.x;
    
    if (v1.x<min) min = v1.x;
    if (v1.x>max) max = v1.x;
    if (v2.x<min) min = v2.x;
    if (v2.x>max) max = v2.x;
    
    if (min>boxhalfsize.x || max<-boxhalfsize.x) return OUTSIDE;
    
    // test in Y-direction
    
    min = v0.y;
    max = v0.y;
    
    if (v1.y<min) min = v1.y;
    if (v1.y>max) max = v1.y;
    if (v2.y<min) min = v2.y;
    if (v2.y>max) max = v2.y;
    
    if (min>boxhalfsize.y || max<-boxhalfsize.y) return OUTSIDE;
    
    // test in Z-direction
    
    min = v0.z;
    max = v0.z;
    
    if (v1.z<min) min = v1.z;
    if (v1.z>max) max = v1.z;
    if (v2.z<min) min = v2.z;
    if (v2.z>max) max = v2.z;
    
    if (min>boxhalfsize.z || max<-boxhalfsize.z) return OUTSIDE;
    
    //  test if the box intersects the plane of the triangle
    
    // test x
    
    if (normal.x>0.0f)
    {
        vmin.x = -boxhalfsize.x;
        vmax.x =  boxhalfsize.x;
    }
    else
    {
        vmin.x =  boxhalfsize.x;
        vmax.x = -boxhalfsize.x;
    }
    
    // test y
    
    if (normal.y>0.0f)
    {
        vmin.y = -boxhalfsize.y;
        vmax.y =  boxhalfsize.y;
    }
    else
    {
        vmin.y =  boxhalfsize.y;
        vmax.y = -boxhalfsize.y;
    }
    
    // test z
    
    if (normal.z>0.0f)
    {
        vmin.z = -boxhalfsize.z;
        vmax.z =  boxhalfsize.z;
    }
    else
    {
        vmin.z =  boxhalfsize.z;
        vmax.z = -boxhalfsize.z;
    }
    
    // test max and min
    
    d = -(normal.x * v0.x + normal.y * v0.y + normal.z * v0.z);
    
    if (normal.x * vmin.x + normal.y * vmin.y + normal.z * vmin.z + d >  0.0f) return OUTSIDE;
    
    if (normal.x * vmax.x + normal.y * vmax.y + normal.z * vmax.z + d >= 0.0f) return INTERSECT;
    
    // box and triangle don't overlap
    
    return OUTSIDE;
}

//

RayCaster::RayCaster(int numSlices, int numPointsSlice)
{
    mCamAngle0 = 0.0;
    mCamAngle1 = 0.0;
    
    mColorMap = NULL;
    
    SetColorMap(0);
    
    mInvertColormap = false;
    
    mNumSlices = numSlices;
    
    // Selection
    mSelectionEnabled = false;
    
    mVolumeSelectionEnabled = false;
    
    mWhitePixImg = -1;
    
    mDisplayRefreshRate = 1;
    mRefreshNum = 0;
    
    mQuality = 1.0;
    
    mPointScale = 1.0;
    
    mAlphaScale = 1.0;
    
    // Keep internally
    mViewWidth = 0;
    mViewHeight = 0;
    
#if PROFILE_RENDER
    BlaTimer::Reset(&mTimer0, &mCount0);
#endif
}

RayCaster::~RayCaster()
{
    if (mColorMap != NULL)
        delete mColorMap;
}

void
RayCaster::SetNumSlices(long numSlices)
{
    mNumSlices = numSlices;
}

long
RayCaster::GetNumSlices()
{
    return mNumSlices;
}

void
RayCaster::PreDraw(NVGcontext *vg, int width, int height)
{
#if PROFILE_RENDER
    BlaTimer::Start(&mTimer0);
#endif
    
    mViewWidth = width;
    mViewHeight = height;
    
    // Optim: GOOD
    // Do not re-allocate points each time
    // Keep a buffer
    
    //vector<Point> points;
    DequeToVec(&mTmpPoints/*&points*/, mSlices);
    vector<Point> &points = mTmpPoints;
    
    //long numPointsStart = points.size();
    
    RayCast(&points, width, height);
    
    //long numPointsEnd = points.size();
    nvgSave(vg);
    
    for (int i = 0; i < points.size(); i++)
    {
        const Point &p = points[i];
    
        NVGcolor color;
        ComputePackedPointColor(p, &color);
        
        nvgStrokeColor(vg, color);
        nvgFillColor(vg, color);
        
        double x = p.mX;
        double y = p.mY;
            
        // Quad rendering
        //
        // (maybe reverse order)
        //
        double size = p.mSize*mPointScale;
        double corners[4][2] = { { x - size/2.0, y - size/2.0 },
                                 { x + size/2.0, y - size/2.0 },
                                 { x + size/2.0, y + size/2.0 },
                                 { x - size/2.0, y + size/2.0 } };
            
        if (mWhitePixImg < 0)
        {
            unsigned char white[4] = { 255, 255, 255, 255 };
            mWhitePixImg = nvgCreateImageRGBA(vg,
                                              1, 1,
                                              NVG_IMAGE_NEAREST,
                                              white);
        }
            
        nvgQuad(vg, corners, mWhitePixImg);
    }
    
    nvgRestore(vg);
    
    // TODO: extract the drawing of the volume rendering in a separate function
    // (to have a clean PreDraw() )
    // Selection
    DrawSelection(vg, width, height);
    
    DBG_DrawSelectionVolume(vg, width, height);
    
#if PROFILE_RENDER
    BlaTimer::StopAndDump(&mTimer0, &mCount0,
                          "profile.txt",
                          "full: %ld ms \n");
#endif
}

void
RayCaster::ClearSlices()
{
    mSlices.clear();
}

void
RayCaster::SetSlices(const vector<vector<Point> > &slices)
{
    mSlices.clear();
    
    for (int i = 0; i < slices.size(); i++)
        mSlices.push_back(slices[i]);
    
    // Could be improved
    while (mSlices.size() > mNumSlices)
        mSlices.pop_front();
    
    UpdateSlicesZ();
}

void
RayCaster::AddSlice(const vector<Point> &points, bool skipDisplay)
{
    vector<Point> points0 = points;
    
    if (skipDisplay)
        return;
    
#if !DISABLE_DISPLAY_REFRESH_RATE
    if (mRefreshNum++ % mDisplayRefreshRate != 0)
        // Don't add the points this time
        return;
#endif
    
    mSlices.push_back(points0);
    
    while (mSlices.size() > mNumSlices)
        mSlices.pop_front();
    
    UpdateSlicesZ();
}

void
RayCaster::SetCameraAngles(double angle0, double angle1)
{
    mCamAngle0 = angle0;
    mCamAngle1 = angle1;
}

void
RayCaster::ProjectPoints(vector<Point> *points, int width, int height)
{
    glm::mat4 model;
    glm::mat4 proj;
    CameraModelProjMat(width, height, mCamAngle0, mCamAngle1,
                       &model, &proj);
    
    glm::mat4 modelProjMat = proj*model;
    
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
        // Matrix transform
        glm::vec4 v;
        v.x = p.mX;
        v.y = p.mY;
        v.z = p.mZ;
        v.w = 1.0;
        
        glm::vec4 v4 = modelProjMat*v;
        
        double x = v4.x;
        double y = v4.y;
        double z = v4.z;
        double w = v4.w;
        
#define EPS 1e-8
        if (fabs(w) > EPS)
        {
            // Optim
            double wInv = 1.0/w;
            x *= wInv;
            y *= wInv;
            z *= wInv;
        }
        
        // Do like OpenGL
        x = (x + 1.0)*0.5*width;
        y = (y + 1.0)*0.5*height;
        z = (z + 1.0f)*0.5;
        
        // Result
        p.mX = x;
        p.mY = y;
        p.mZ = z;
    }
}

void
RayCaster::RayCast(vector<Point> *points, int width, int height)
{    
    ApplyColorMap(points);
    
    ProjectPoints(points, width, height);

    // NOTE: - when no selection, consume few additional performances
    //       - when (large) selection, consume as much as ProjectPoints()
    SelectPoints(points, width, height);
    PropagateSelection(*points);

    DoRayCast(points, width, height);
}

void
RayCaster::DoRayCast(vector<Point> *points, int width, int height)
{
    int res = GetResolution();
    int resolution[2] = { res, res };
    
    double maxDim = (width > height) ? width : height;
    double bbox[2][2] = { { 0, 0 }, { maxDim, maxDim } };
    
    // Create the grid
    if (mTmpGrid.size() != resolution[0])
    {
        mTmpGrid.resize(resolution[0]);
        for (int i = 0; i < resolution[0]; i++)
        {
            mTmpGrid[i].resize(resolution[1]);
        }
    }
    
    // Empty the grid
    for (int i = 0; i < resolution[0]; i++)
    {
        for (int j = 0; j < resolution[1]; j++)
        {
            mTmpGrid[i][j].clear();
        }
    }
    
    vector<vector<vector<Point> > > &grid = mTmpGrid;
        
    // Fill the grid
    
    // Optim
    double xRatio = 1.0/((bbox[1][0] - bbox[0][0]));
    double yRatio = 1.0/((bbox[1][1] - bbox[0][1]));
    for (int i = 0; i < points->size(); i++)
    {
        const Point &p = (*points)[i];
        
        double normX = (p.mX - bbox[0][0])*xRatio;
        int x = (int)(normX*resolution[0]);
        
        double normY = (p.mY - bbox[0][1])*yRatio;
        int y = (int)(normY*resolution[1]);
        
        // Test to avoid crash when projected points are outside the screen
        if ((x >= 0) && (x < resolution[0]) &&
            (y >= 0) && (y < resolution[1]))
        {
            // Add the point
            grid[x][y].push_back(p);
        }
    }
    
    // Compute the grid cells color
    double cellSize = (bbox[1][0] - bbox[0][0])/resolution[0];
    
    // Clear the result
    points->clear();
    
    double x = 0;
    double y = 0;
    for (int i = 0; i < grid.size(); i++)
    {
        vector<vector<Point> > &line = grid[i];
        
        for (int j = 0; j < line.size(); j++)
        {
            vector<Point> &points0 = line[j];
            
            if (points0.empty())
            {
                y += cellSize;
                
                continue;
            }
            
#if 0 // Sort, then process the list (takes more resources)
      //
            // Get a single point from the point list for this cell
            sort(points0.begin(), points0.end(), Point::IsSmaller);
            
            for (int k = 0; k < points0.size(); k++)
            //for (int k = points0.size() - 1; k >= 0; k--)
            {
                Point &p = points0[k];
            
#define EPS_ALPHA 1
                // Except if it is transparent
                if (p.mA < EPS_ALPHA)
                    continue;
                
                //p.mX = i*cellSize;
                //p.mY = j*cellSize;
            
                // OPTIM (not very efficient)
                p.mX = x;
                p.mY = y;
                
                p.mSize = cellSize;
            
                points->push_back(p);
                
                // Simply get the nearest point (for the moment)
                break;
            }
#endif
            
            // OPTIM: GOOD (30% better)
            // Find the point with minimum Z
            // (instead of sorting)
            
#if 1 // Do not sort, take min z !
            
#define INF 1e15
            Point *minPoint = NULL;
            double minZ = INF;
            
            for (int k = 0; k < points0.size(); k++)
            {
                Point &p = points0[k];
                
#define EPS_ALPHA 1
                // Ignore if it is transparent
                if (p.mA < EPS_ALPHA)
                    continue;
                
                if (p.mZ < minZ)
                {
                    minPoint = &p;
                    minZ = p.mZ;
                }
            }
            
            // If point found, add it to the list
            if (minPoint != NULL)
            {
                minPoint->mX = x;
                minPoint->mY = y;
                
                minPoint->mSize = cellSize;
                
                points->push_back(*minPoint);
            }
#endif
            
            y += cellSize;
        }
        
        y = 0.0;
        x += cellSize;
    }
}

void
RayCaster::ComputePackedPointColor(const RayCaster::Point &p, NVGcolor *color)
{
    // Optim: pre-compute nvg color
    unsigned char color0[4] = { p.mR, p.mG, p.mB, p.mA };
    
    if (mSelectionEnabled)
    {
        // If not selected, gray out
        if (!p.mIsSelected)
        {
            for (int i = 0; i < 3; i++)
                color0[i] *= SELECT_GRAY_COLOR_COEFF;
        }
    }
    
    color0[3] *= mAlphaScale;
    
    if (color0[3] > 255)
        color0[3] = 255;
    
    SWAP_COLOR(color0);
    
    *color = nvgRGBA(color0[0], color0[1], color0[2], color0[3]);
}

void
RayCaster::SetColorMap(int colormapId)
{
    switch(colormapId)
    {
        case 0:
        {
            if (mColorMap != NULL)
                delete mColorMap;
    
            // Blue and dark pink
            mColorMap = new ColorMap4(false);
            
#if TRANSPARENT_COLORMAP
            mColorMap->AddColor(0, 0, 0, 0/*255*/, 0.0);
#else
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
#endif
            
            mColorMap->AddColor(68, 22, 68, 255, 0.25);
            mColorMap->AddColor(32, 122, 190, 255, 0.5);
            mColorMap->AddColor(172, 212, 250, 255, 0.75);
            
            mColorMap->AddColor(255, 255, 255, 255, 1.0);
        }
        break;
            
        case 1:
        {
            if (mColorMap != NULL)
                delete mColorMap;
            
            // Dawn (thermal...)
            mColorMap = new ColorMap4(false);
            mColorMap->AddColor(4, 35, 53, 255, 0.0);
            
            mColorMap->AddColor(90, 61, 154, 255, 0.25);
            mColorMap->AddColor(185, 97, 125, 255, 0.5);
            
            mColorMap->AddColor(245, 136, 71, 255, 0.75);
            
            mColorMap->AddColor(233, 246, 88, 255, 1.0);
        }
        break;
            
        case 2:
        {
            if (mColorMap != NULL)
                delete mColorMap;
            
            // Multicolor ("jet")
            mColorMap = new ColorMap4(false);
            mColorMap->AddColor(0, 0, 128, 255, 0.0);
            mColorMap->AddColor(0, 0, 246, 255, 0.1);
            mColorMap->AddColor(0, 77, 255, 255, 0.2);
            mColorMap->AddColor(0, 177, 255, 255, 0.3);
            mColorMap->AddColor(38, 255, 209, 255, 0.4);
            mColorMap->AddColor(125, 255, 122, 255, 0.5);
            mColorMap->AddColor(206, 255, 40, 255, 0.6);
            mColorMap->AddColor(255, 200, 0, 255, 0.7);
            mColorMap->AddColor(255, 100, 0, 255, 0.8);
            mColorMap->AddColor(241, 8, 0, 255, 0.9);
            mColorMap->AddColor(132, 0, 0, 255, 1.0);
        }
        break;
            
        default:
            break;
    }
    
    if (mColorMap != NULL)
        mColorMap->Generate();
    
#if 0
    // Sky (ice)
    mColorMap = new ColorMap4(false);
    mColorMap->AddColor(4, 6, 19, 255, 0.0);
    
    mColorMap->AddColor(58, 61, 126, 255, 0.25);
    mColorMap->AddColor(67, 126, 184, 255, 0.5);
    
    mColorMap->AddColor(73, 173, 208, 255, 0.75);
    
    mColorMap->AddColor(232, 251, 252, 255, 1.0);
#endif
}

void
RayCaster::SetInvertColormap(bool flag)
{
    mInvertColormap = flag;
}

void
RayCaster::SetColormapRange(double range)
{
    if (mColorMap != NULL)
    {
        mColorMap->SetRange(range);
    }
}

void
RayCaster::SetColormapContrast(double contrast)
{
    if (mColorMap != NULL)
    {
        mColorMap->SetContrast(contrast);
    }
}

void
RayCaster::GetSelection(double selection[4])
{
    for (int i = 0; i < 4; i++)
        selection[i] = mScreenSelection[i];
    
    ReorderScreenSelection(selection);
}

void
RayCaster::SetSelection(const double selection[4])
{
    for (int i = 0; i < 4; i++)
        mScreenSelection[i] = selection[i];
    
    // Re-order the selection
    if (mScreenSelection[2] < mScreenSelection[0])
    {
        double tmp = mScreenSelection[2];
        mScreenSelection[2] = mScreenSelection[0];
        mScreenSelection[0] = tmp;
    }
    
    if (mScreenSelection[3] < mScreenSelection[1])
    {
        double tmp = mScreenSelection[3];
        mScreenSelection[3] = mScreenSelection[1];
        mScreenSelection[1] = tmp;
    }
    
    mSelectionEnabled = true;
    
    
    SelectionToVolume();
    
    mVolumeSelectionEnabled = true;
}

void
RayCaster::EnableSelection()
{
    mSelectionEnabled = true;
}

void
RayCaster::DisableSelection()
{
    mSelectionEnabled = false;
}

bool
RayCaster::SelectionEnabled()
{
    return mSelectionEnabled;
}

bool
RayCaster::GetCenterSliceSelection(vector<bool> *selection,
                                   vector<double> *xCoords,
                                   long *sliceNum)
{
    // Cannot simply take the extremity selected points
    // (it is unstable and selected slice num jitters)
    
    // Make some intersection tests to have the center slice
    long centerSlice = GetCenterSlice();
    
    *sliceNum = centerSlice;
    
    if ((centerSlice < 0.0) || (centerSlice >= mSlices.size()))
        // Just in case
        return false;
    
    const vector<Point> &slice = mSlices[centerSlice];
    
    // Fill selection flags and x coords
    selection->resize(slice.size());
    xCoords->resize(slice.size());
    
    for (int i = 0; i < slice.size(); i++)
    {
        const Point &p = slice[i];
        
        bool selected = p.mIsSelected;
        (*selection)[i] = selected;
        
        double x = p.mX;
        (*xCoords)[i] = x;
    }
    
    return true;
}

bool
RayCaster::SelectionIntersectSlice2D(const double selection2D[2][2],
                                     const double projSlice2D[4][2])
{
    glm::vec3 bmin(selection2D[0][0], selection2D[0][1], 0.0);
    glm::vec3 bmax(selection2D[1][0], selection2D[1][1], 0.0);
    glm::vec3 boxcenter((bmin + bmax)/2.0f);
    glm::vec3 boxhalfsize((bmax - bmin)/2.0f);
    
    for (int i = 0; i < 2; i++)
    {
        // Test with both normals
        glm::vec3 normal(0.0, 0.0, 1.0);
        if (i == 1)
            normal *= -1.0;
        
        glm::vec3 trivert0(projSlice2D[0][0], projSlice2D[0][1], 0.0);
        glm::vec3 trivert1(projSlice2D[1][0], projSlice2D[1][1], 0.0);
        glm::vec3 trivert2(projSlice2D[2][0], projSlice2D[2][1], 0.0);
        glm::vec3 trivert3(projSlice2D[3][0], projSlice2D[3][1], 0.0);
        
        // First triangle
        int intersect = TriangleVsAABB2(bmin, bmax,
                                        boxcenter, boxhalfsize,
                                        trivert0, trivert1, trivert2,
                                        normal);
        
        if (intersect != OUTSIDE)
            return true;
        
        // Second triangle
        intersect = TriangleVsAABB2(bmin, bmax,
                                    boxcenter, boxhalfsize,
                                    trivert0, trivert2, trivert3,
                                    normal);
        
        if (intersect != OUTSIDE)
            return true;
    }
    
    return false;
}

long
RayCaster::GetCenterSlice()
{
    // Project each slice, and make slice (2 triangles) intersection with
    // the selection axis aligned bounding box (in 2d)
    
    long minSlice = mSlices.size() - 1;
    long maxSlice = 0;
    
    double selection[4] = { mScreenSelection[0]*mViewWidth + 4.0, (1.0 - mScreenSelection[3])*mViewHeight + 4.0,
                            mScreenSelection[2]*mViewWidth + 1.0, (1.0 - mScreenSelection[1])*mViewHeight + 1.0 };
    
    double selection2D[2][2] = { { selection[0], selection[1] },
                                 { selection[2], selection[3] } };
        
        
    
    for (int i = 0; i < mSlices.size(); i++)
    {
        // Compute time step
        double z = 1.0 - ((double)i)/mSlices.size();
        
        // Center
        z -= 0.5;
        
        // Slice in world space
        double slice[4][3] = { { -0.5, -0.5, z },
                               { 0.5, -0.5, z },
                               { 0.5, 0.5, z },
                               { -0.5, 0.5, z } };
        
        // Project the slice
        double projSlice[4][3];
        for (int k = 0; k < 4; k++)
        {
            ProjectPoint(projSlice[k], slice[k], mViewWidth, mViewHeight);
        }
        
        double projSlice2D[4][2];
        for (int k = 0; k < 4; k++)
        {
            projSlice2D[k][0] = projSlice[k][0];
            projSlice2D[k][1] = projSlice[k][1];
        }
        
        bool intersect = SelectionIntersectSlice2D(selection2D, projSlice2D);
        if (intersect)
        {
            if (i < minSlice)
                minSlice = i;
                
            if (i > maxSlice)
                maxSlice = i;
        }
    }
    
    if (minSlice > maxSlice)
        // Failed
        return -1;
    
    long res = (minSlice + maxSlice)/2;
    
    // DEBUG
    //DBG_SelectSlice(res);
    
    return res;
}

void
RayCaster::SelectionToVolume()
{
#define EPS 1e-15
    
    double screenSelection[4];
    ScreenToAlignedVolume(mScreenSelection, screenSelection);
    
    // TEST
    //ReorderScreenSelection(screenSelection);
    
    if ((fabs(mCamAngle0) < EPS) && (fabs(mCamAngle1) < EPS))
        // Front face
    {
        double zMin = 0.5; //-0.5; //0.0; //-0.5;
        double zMax = -0.5; //0.5; //1.0; //0.5;
        
        if (mVolumeSelectionEnabled)
        {
            zMin = mVolumeSelection[0].mZ;
            zMax = mVolumeSelection[4].mZ;
        }
        
        mVolumeSelection[0] = SelectionPoint(screenSelection[0], screenSelection[1], zMin);
        mVolumeSelection[1] = SelectionPoint(screenSelection[2], screenSelection[1], zMin);
        mVolumeSelection[2] = SelectionPoint(screenSelection[2], screenSelection[3], zMin);
        mVolumeSelection[3] = SelectionPoint(screenSelection[0], screenSelection[3], zMin);
        
        mVolumeSelection[4] = SelectionPoint(screenSelection[0], screenSelection[1], zMax);
        mVolumeSelection[5] = SelectionPoint(screenSelection[2], screenSelection[1], zMax);
        mVolumeSelection[6] = SelectionPoint(screenSelection[2], screenSelection[3], zMax);
        mVolumeSelection[7] = SelectionPoint(screenSelection[0], screenSelection[3], zMax);
    }
    
    if ((mCamAngle0 > EPS) && (fabs(mCamAngle1) < EPS))
        // Left face
    {
        double xMin = -0.5; //0.0;
        double xMax = 0.5; //1.0;
        
        if (mVolumeSelectionEnabled)
        {
            xMin = mVolumeSelection[0/*4*//*0*/].mX;
            xMax = mVolumeSelection[1/*3*//*1*/].mX;
        }
        
        mVolumeSelection[0] = SelectionPoint(xMin, screenSelection[1], screenSelection[2]);
        mVolumeSelection[1] = SelectionPoint(xMax, screenSelection[1], screenSelection[2]);
        mVolumeSelection[2] = SelectionPoint(xMax, screenSelection[3], screenSelection[2]);
        mVolumeSelection[3] = SelectionPoint(xMin, screenSelection[3], screenSelection[2]);
        
        mVolumeSelection[4] = SelectionPoint(xMin, screenSelection[1], screenSelection[0]);
        mVolumeSelection[5] = SelectionPoint(xMax, screenSelection[1], screenSelection[0]);
        mVolumeSelection[6] = SelectionPoint(xMax, screenSelection[3], screenSelection[0]);
        mVolumeSelection[7] = SelectionPoint(xMin, screenSelection[3], screenSelection[0]);
    }
    
    if ((mCamAngle0 < -EPS) && (fabs(mCamAngle1) < EPS))
        // Right face
    {
        double xMin = -0.5; //0.0;
        double xMax = 0.5; //1.0;
        
        if (mVolumeSelectionEnabled)
        {
            xMin = mVolumeSelection[0/*1*//*0*/].mX;
            xMax = mVolumeSelection[1/*6*//*1*/].mX;
        }
        
        mVolumeSelection[0] = SelectionPoint(xMin, screenSelection[1], screenSelection[0]);
        mVolumeSelection[1] = SelectionPoint(xMax, screenSelection[1], screenSelection[0]);
        mVolumeSelection[2] = SelectionPoint(xMax, screenSelection[3], screenSelection[0]);
        mVolumeSelection[3] = SelectionPoint(xMin, screenSelection[3], screenSelection[0]);
        
        mVolumeSelection[4] = SelectionPoint(xMin, screenSelection[1], screenSelection[2]);
        mVolumeSelection[5] = SelectionPoint(xMax, screenSelection[1], screenSelection[2]);
        mVolumeSelection[6] = SelectionPoint(xMax, screenSelection[3], screenSelection[2]);
        mVolumeSelection[7] = SelectionPoint(xMin, screenSelection[3], screenSelection[2]);
    }
    
    if ((fabs(mCamAngle0) < EPS) && (mCamAngle1 > EPS))
        // Top face front
    {
        double yMin = 0.0;
        double yMax = 1.0;
        
        if (mVolumeSelectionEnabled)
        {
            yMin = mVolumeSelection[0].mY;
            yMax = mVolumeSelection[2].mY;
        }
        
        mVolumeSelection[0] = SelectionPoint(screenSelection[0], yMin, screenSelection[1]);
        mVolumeSelection[1] = SelectionPoint(screenSelection[2], yMin, screenSelection[1]);
        mVolumeSelection[2] = SelectionPoint(screenSelection[2], yMax, screenSelection[1]);
        mVolumeSelection[3] = SelectionPoint(screenSelection[0], yMax, screenSelection[1]);
        
        mVolumeSelection[4] = SelectionPoint(screenSelection[0], yMin, screenSelection[3]);
        mVolumeSelection[5] = SelectionPoint(screenSelection[2], yMin, screenSelection[3]);
        mVolumeSelection[6] = SelectionPoint(screenSelection[2], yMax, screenSelection[3]);
        mVolumeSelection[7] = SelectionPoint(screenSelection[0], yMax, screenSelection[3]);
    }
    
    if ((mCamAngle0 > EPS) && (mCamAngle1 > EPS))
        // Top face left
    {
        double yMin = 0.0; //-0.5; //0.0;
        double yMax = 1.0; //0.5; //1.0;
        
        if (mVolumeSelectionEnabled)
        {
            yMin = mVolumeSelection[0].mY; //mZ; //mY;
            yMax = mVolumeSelection[3/*4*//*2*/].mY; //mZ; //mY;
        }
        
        mVolumeSelection[0] = SelectionPoint(1.0 - screenSelection[3], yMin, screenSelection[0]);
        mVolumeSelection[1] = SelectionPoint(1.0 - screenSelection[1], yMin, screenSelection[0]);
        mVolumeSelection[2] = SelectionPoint(1.0 - screenSelection[1], yMax, screenSelection[0]);
        mVolumeSelection[3] = SelectionPoint(1.0 - screenSelection[3], yMax, screenSelection[0]);
        
        mVolumeSelection[4] = SelectionPoint(1.0 - screenSelection[3], yMin, screenSelection[2]);
        mVolumeSelection[5] = SelectionPoint(1.0 - screenSelection[1], yMin, screenSelection[2]);
        mVolumeSelection[6] = SelectionPoint(1.0 - screenSelection[1], yMax, screenSelection[2]);
        mVolumeSelection[7] = SelectionPoint(1.0 - screenSelection[3], yMax, screenSelection[2]);
    }
    
    if ((mCamAngle0 < -EPS) && (mCamAngle1 > EPS))
        // Top face right
    {
        double yMin = 0.0; //-0.5; //0.0;
        double yMax = 1.0; //0.5; //1.0;
        
        if (mVolumeSelectionEnabled)
        {
            yMin = mVolumeSelection[0].mY; //mZ; //mY;
            yMax = mVolumeSelection[3/*4*//*2*/].mY; //mZ; //mY;
        }
        
        mVolumeSelection[0] = SelectionPoint(screenSelection[1], yMin, 1.0 - screenSelection[2]);
        mVolumeSelection[1] = SelectionPoint(screenSelection[3], yMin, 1.0 - screenSelection[2]);
        mVolumeSelection[2] = SelectionPoint(screenSelection[3], yMax, 1.0 - screenSelection[2]);
        mVolumeSelection[3] = SelectionPoint(screenSelection[1], yMax, 1.0 - screenSelection[2]);
        
        mVolumeSelection[4] = SelectionPoint(screenSelection[1], yMin, 1.0 - screenSelection[0]);
        mVolumeSelection[5] = SelectionPoint(screenSelection[3], yMin, 1.0 - screenSelection[0]);
        mVolumeSelection[6] = SelectionPoint(screenSelection[3], yMax, 1.0 - screenSelection[0]);
        mVolumeSelection[7] = SelectionPoint(screenSelection[1], yMax, 1.0 - screenSelection[0]);
    }
}

void
RayCaster::VolumeToSelection()
{
#define EPS 1e-15
    
    double screenSelection[4];
    
    if ((fabs(mCamAngle0) < EPS) && (fabs(mCamAngle1) < EPS))
        // Front face
    {
        screenSelection[0] = mVolumeSelection[0/*4*/].mX;
        screenSelection[1] = mVolumeSelection[0/*4*/].mY;
        
        screenSelection[2] = mVolumeSelection[2/*6*/].mX;
        screenSelection[3] = mVolumeSelection[2/*6*/].mY;
    }
    
    if ((mCamAngle0 > EPS) && (fabs(mCamAngle1) < EPS))
        // Left face
    {
        screenSelection[0] = mVolumeSelection[4].mZ;
        screenSelection[1] = mVolumeSelection[4].mY;
        
        screenSelection[2] = mVolumeSelection[3].mZ;
        screenSelection[3] = mVolumeSelection[3].mY;
    }
    
    if ((mCamAngle0 < -EPS) && (fabs(mCamAngle1) < EPS))
        // Right face
    {
        screenSelection[0] = /*1.0 -*/ mVolumeSelection[1].mZ;
        screenSelection[1] = mVolumeSelection[1].mY;
        
        screenSelection[2] = /*1.0 -*/ mVolumeSelection[6].mZ;
        screenSelection[3] = mVolumeSelection[6].mY;
    }
    
    if ((fabs(mCamAngle0) < EPS) && (mCamAngle1 > EPS))
        // Top face front
    {
        screenSelection[0] = mVolumeSelection[3/*0*/].mX;
        screenSelection[1] = mVolumeSelection[3/*0*/].mZ;
        
        screenSelection[2] = mVolumeSelection[6].mX;
        screenSelection[3] = mVolumeSelection[6].mZ;
    }
    
    if ((mCamAngle0 < -EPS) && (mCamAngle1 > EPS))
        // Top face left
    {
        screenSelection[0] = 1.0 - mVolumeSelection[7].mZ;
        screenSelection[1] = mVolumeSelection[7].mX;
        
        screenSelection[2] = 1.0 - mVolumeSelection[2].mZ;
        screenSelection[3] = mVolumeSelection[2].mX;
    }
    
    if ((mCamAngle0 > EPS) && (mCamAngle1 > EPS))
        // Top face right
    {
        screenSelection[0] = mVolumeSelection[2].mZ;
        screenSelection[1] = 1.0 - mVolumeSelection[2].mX;
        
        screenSelection[2] = mVolumeSelection[7/*3*/].mZ;
        screenSelection[3] = 1.0 - mVolumeSelection[7/*3*/].mX;
    }
    
    ReorderScreenSelection(screenSelection);
    
    AlignedVolumeToScreen(screenSelection, mScreenSelection);
}

void
RayCaster::ScreenToAlignedVolume(const double screenSelectionNorm[4],
                                 double alignedVolumeSelection[4])
{
    double projFace[2][3];
    ProjectFace(projFace);
    
    double screenSelection[4] = { screenSelectionNorm[0],
                                  1.0 - screenSelectionNorm[1],
                                  screenSelectionNorm[2],
                                  1.0 - screenSelectionNorm[3] };
    
    screenSelection[0] *= mViewWidth;
    screenSelection[1] *= mViewHeight;
    screenSelection[2] *= mViewWidth;
    screenSelection[3] *= mViewHeight;
    
    alignedVolumeSelection[0] = (screenSelection[0] - projFace[0][0])/(projFace[1][0] - projFace[0][0]);
    alignedVolumeSelection[1] = (screenSelection[1] - projFace[0][1])/(projFace[1][1] - projFace[0][1]);
    
    alignedVolumeSelection[2] = (screenSelection[2] - projFace[0][0])/(projFace[1][0] - projFace[0][0]);
    alignedVolumeSelection[3] = (screenSelection[3] - projFace[0][1])/(projFace[1][1] - projFace[0][1]);
}

#if 0
void
RayCaster::AlignedVolumeToScreen(const double alignedVolumeSelection[4],
                                 double screenSelectionNorm[4])
{
    double projFace[2][3];
    ProjectFace(projFace);
    
    double screenSelection[4];
    
    screenSelection[0] = alignedVolumeSelection[0]*(projFace[1][0] - projFace[0][0]) + projFace[0][0];
    screenSelection[1] = alignedVolumeSelection[1]*(projFace[1][1] - projFace[0][1]) + projFace[0][1];
    
    screenSelection[2] = alignedVolumeSelection[2]*(projFace[1][0] - projFace[0][0]) + projFace[0][0];
    screenSelection[3] = alignedVolumeSelection[3]*(projFace[1][1] - projFace[0][1]) + projFace[0][1];
    
    screenSelectionNorm[0] = screenSelection[0]/mViewWidth;
    screenSelectionNorm[1] = 1.0 - screenSelection[1]/mViewHeight;
    screenSelectionNorm[2] = screenSelection[2]/mViewWidth;
    screenSelectionNorm[3] = 1.0 - screenSelection[3]/mViewHeight;
}
#endif

// Align selection to colume nearest face
void
RayCaster::AlignedVolumeToScreen(const double alignedVolumeSelection[4],
                                 double screenSelectionNorm[4])
{
    double projFace[2][3];
    
    if ((fabs(mCamAngle0) < EPS) && (fabs(mCamAngle1) < EPS))
        // Front face
    {
        double face[2][3] = { { mVolumeSelection[0/*4*/].mX, mVolumeSelection[0/*4*/].mY, mVolumeSelection[0/*4*/].mZ },
                              { mVolumeSelection[2/*6*/].mX, mVolumeSelection[2/*6*/].mY, mVolumeSelection[2/*6*/].mZ } };
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 > EPS) && (fabs(mCamAngle1) < EPS))
        // Left face
    {
        double face[2][3] = { { mVolumeSelection[4].mX, mVolumeSelection[4].mY, mVolumeSelection[4].mZ },
                              { mVolumeSelection[3].mX, mVolumeSelection[3].mY, mVolumeSelection[3].mZ } };
        
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 < -EPS) && (fabs(mCamAngle1) < EPS))
        // Right face
    {
        double face[2][3] = { { mVolumeSelection[1].mX, mVolumeSelection[1].mY, mVolumeSelection[1].mZ },
                              { mVolumeSelection[6].mX, mVolumeSelection[6].mY, mVolumeSelection[6].mZ } };
        
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((fabs(mCamAngle0) < EPS) && (mCamAngle1 > EPS))
        // Top face front
    {
        double face[2][3] = { { mVolumeSelection[3/*0*/].mX, mVolumeSelection[3/*0*/].mY, mVolumeSelection[3/*0*/].mZ },
                              { mVolumeSelection[6/*5*/].mX, mVolumeSelection[6/*5*/].mY, mVolumeSelection[6/*5*/].mZ } };
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 < -EPS) && (mCamAngle1 > EPS))
        // Top face left
    {
        double face[2][3] = { { mVolumeSelection[4].mX, mVolumeSelection[4].mY, mVolumeSelection[4].mZ },
                              { mVolumeSelection[1].mX, mVolumeSelection[1].mY, mVolumeSelection[1].mZ } };
        
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 > EPS) && (mCamAngle1 > EPS))
        // Top face right
    {
        double face[2][3] = { { mVolumeSelection[1].mX, mVolumeSelection[1].mY, mVolumeSelection[1].mZ },
                              { mVolumeSelection[4].mX, mVolumeSelection[4].mY, mVolumeSelection[4].mZ } };
        
        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    double screenSelection[4] = { projFace[0][0], projFace[0][1],
                                  projFace[1][0], projFace[1][1] };
    
    screenSelectionNorm[0] = screenSelection[0]/mViewWidth;
    screenSelectionNorm[1] = 1.0 - screenSelection[1]/mViewHeight;
    screenSelectionNorm[2] = screenSelection[2]/mViewWidth;
    screenSelectionNorm[3] = 1.0 - screenSelection[3]/mViewHeight;
}

void
RayCaster::ProjectFace(double projFace[2][3])
{
    if ((fabs(mCamAngle0) < EPS) && (fabs(mCamAngle1) < EPS))
        // Front face
    {
        //double face[2][3] = { { -0.5, -0.5, -0.5 },
        //                      { 0.5, 0.5, -0.5 } };
        
        double face[2][3] = { { 0.0, 0.0, 0.5/*1.0*//*-0.5*//*0.0*/ },
                              { 1.0, 1.0, 0.5/*1.0*//*-0.5*//*0.0*/ } };
            
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 > EPS) && (fabs(mCamAngle1) < EPS))
        // Left face
    {
        //double face[2][3] = { { -0.5, -0.5, -0.5 },
        //                      { -0.5, 0.5, 0.5 } };
        
        double face[2][3] = { { -0.5, 0.0, 0.0 },
                              { -0.5, 1.0, 1.0 } };

        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 < -EPS) && (fabs(mCamAngle1) < EPS))
        // Right face
    {
        //double face[2][3] = { { 0.5, -0.5, -0.5 },
        //                      { 0.5, 0.5, 0.5 } };
        
        double face[2][3] = { { 1.0, 0.0, 0.0 },
                              { 1.0, 1.0, 1.0 } };

        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((fabs(mCamAngle0) < EPS) && (mCamAngle1 > EPS))
        // Top face front
    {
        //double face[2][3] = { { -0.5, 0.5, -0.5 },
        //                      { 0.5, 0.5, 0.5 } };
        
        double face[2][3] = { { 0.0, 1.0, 0.0 },
                              { 1.0, 1.0, 1.0 } };
            
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 < -EPS) && (mCamAngle1 > EPS))
        // Top face left
    {
        //double face[2][3] = { { -0.5, 0.5, 0.5 },
        //                      { 0.5, 0.5, -0.5 } };
        double face[2][3] = { { 0.0, 1.0, 1.0 },
                              { 1.0, 1.0, 0.0 } };

        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
    
    if ((mCamAngle0 > EPS) && (mCamAngle1 > EPS))
        // Top face right
    {
        //double face[2][3] = { { 0.5, 0.5, -0.5 },
        //                      { -0.5, 0.5, 0.5 } };
        double face[2][3] = { { 1.0, 1.0, 0.0 },
                              { 0.0, 1.0, 1.0 } };

        
        ProjectPoint(projFace[0], face[0], mViewWidth, mViewHeight);
        ProjectPoint(projFace[1], face[1], mViewWidth, mViewHeight);
    }
}

void
RayCaster::ReorderScreenSelection(double screenSelection[4])
{
    // Re-order the selection
    if (screenSelection[2] < screenSelection[0])
    {
        double tmp = screenSelection[2];
        screenSelection[2] = screenSelection[0];
        screenSelection[0] = tmp;
    }
    
    if (screenSelection[3] < screenSelection[1])
    {
        double tmp = screenSelection[3];
        screenSelection[3] = screenSelection[1];
        screenSelection[1] = tmp;
    }
}

void
RayCaster::DBG_SelectSlice(long sliceNum)
{
    if ((sliceNum < 0) || (sliceNum >= mSlices.size()))
        return;
    
    vector<Point> &slice = mSlices[sliceNum];
    for (int i = 0; i < slice.size(); i++)
    {
        Point &p = slice[i];
        p.mIsSelected = true;
    }
}

void
RayCaster::SetDisplayRefreshRate(int displayRefreshRate)
{
#if !DISABLE_DISPLAY_REFRESH_RATE
    mDisplayRefreshRate = displayRefreshRate;
#endif
}

void
RayCaster::SetQuality(double quality)
{
    mQuality = quality;
}

void
RayCaster::SetPointScale(double scale)
{
    mPointScale = scale;
}

void
RayCaster::SetAlphaScale(double scale)
{
    mAlphaScale = scale;
}

void
RayCaster::DrawSelection(NVGcontext *vg, int width, int height)
{
    if (!mSelectionEnabled)
        return;
    
    double strokeWidths[2] = { 3.0, 2.0 };
    
    // Two colors, for drawing two times, for overlay
    int colors[2][4] = { { 64, 64, 64, 255 }, { 255, 255, 255, 255 } };
    
    for (int i = 0; i < 2; i++)
    {
        nvgStrokeWidth(vg, strokeWidths[i]);
        
        SWAP_COLOR(colors[i]);
        nvgStrokeColor(vg, nvgRGBA(colors[i][0], colors[i][1], colors[i][2], colors[i][3]));
        
        // Draw the circle
        nvgBeginPath(vg);
        
#if 0
        // Draw the line
        nvgMoveTo(vg, mScreenSelection[0], height - mScreenSelection[1]);
        
        nvgLineTo(vg, mScreenSelection[2], height - mScreenSelection[1]);
        nvgLineTo(vg, mScreenSelection[2], height - mScreenSelection[3]);
        nvgLineTo(vg, mScreenSelection[0], height - mScreenSelection[3]);
        nvgLineTo(vg, mScreenSelection[0], height - mScreenSelection[1]);
#endif
        
#if 1
        // Draw the line
        nvgMoveTo(vg, mScreenSelection[0]*width, (1.0 - mScreenSelection[1])*height);
        
        nvgLineTo(vg, mScreenSelection[2]*width, (1.0 - mScreenSelection[1])*height);
        nvgLineTo(vg, mScreenSelection[2]*width, (1.0 - mScreenSelection[3])*height);
        nvgLineTo(vg, mScreenSelection[0]*width, (1.0 - mScreenSelection[3])*height);
        nvgLineTo(vg, mScreenSelection[0]*width, (1.0 - mScreenSelection[1])*height);
#endif
        
        nvgStroke(vg);
    }
}

void
RayCaster::DBG_DrawSelectionVolume(NVGcontext *vg, int width, int height)
{
    //if (!mSelectionEnabled)
    //    return;
    
#if 0
    // Full Volume
    double volume[8][3] =
    {   { -0.5, -0.5, -0.5 },
        { 0.5, -0.5, -0.5 },
        { 0.5, 0.5, -0.5 },
        { -0.5, 0.5, -0.5 },
        
        { -0.5, -0.5, 0.5 },
        { 0.5, -0.5, 0.5 },
        { 0.5, 0.5, 0.5 },
        { -0.5, 0.5, 0.5 }
    };
#endif
    
    double volume[8][3] =
    {
        { mVolumeSelection[0].mX, mVolumeSelection[0].mY, mVolumeSelection[0].mZ },
        { mVolumeSelection[1].mX, mVolumeSelection[1].mY, mVolumeSelection[1].mZ },
        { mVolumeSelection[2].mX, mVolumeSelection[2].mY, mVolumeSelection[2].mZ },
        { mVolumeSelection[3].mX, mVolumeSelection[3].mY, mVolumeSelection[3].mZ },
        
        { mVolumeSelection[4].mX, mVolumeSelection[4].mY, mVolumeSelection[4].mZ },
        { mVolumeSelection[5].mX, mVolumeSelection[5].mY, mVolumeSelection[5].mZ },
        { mVolumeSelection[6].mX, mVolumeSelection[6].mY, mVolumeSelection[6].mZ },
        { mVolumeSelection[7].mX, mVolumeSelection[7].mY, mVolumeSelection[7].mZ },
    };
    
    // Projected volume
    double projectedVolume[8][3];
    for (int i = 0; i < 8; i++)
    {
        ProjectPoint(projectedVolume[i], volume[i], width, height);        
    }
    
    // Draw the projected volume
    nvgStrokeWidth(vg, 1.0);
    
    int color[4] = { 255, 255, 255, 255 };
    SWAP_COLOR(color);
    nvgStrokeColor(vg, nvgRGBA(color[0], color[1], color[2], color[3]));
    
    // Draw front face
    nvgBeginPath(vg);
    nvgMoveTo(vg, projectedVolume[0][0], projectedVolume[0][1]);
    nvgLineTo(vg, projectedVolume[1][0], projectedVolume[1][1]);
    nvgLineTo(vg, projectedVolume[2][0], projectedVolume[2][1]);
    nvgLineTo(vg, projectedVolume[3][0], projectedVolume[3][1]);
    nvgLineTo(vg, projectedVolume[0][0], projectedVolume[0][1]);
    nvgStroke(vg);
    
    // Draw back face
    nvgBeginPath(vg);
    nvgMoveTo(vg, projectedVolume[4][0], projectedVolume[4][1]);
    nvgLineTo(vg, projectedVolume[5][0], projectedVolume[5][1]);
    nvgLineTo(vg, projectedVolume[6][0], projectedVolume[6][1]);
    nvgLineTo(vg, projectedVolume[7][0], projectedVolume[7][1]);
    nvgLineTo(vg, projectedVolume[4][0], projectedVolume[4][1]);
    nvgStroke(vg);
    
    // Draw intermediate lines
    nvgBeginPath(vg);
    nvgMoveTo(vg, projectedVolume[0][0], projectedVolume[0][1]);
    nvgLineTo(vg, projectedVolume[4][0], projectedVolume[4][1]);
    nvgStroke(vg);
    
    nvgBeginPath(vg);
    nvgMoveTo(vg, projectedVolume[1][0], projectedVolume[1][1]);
    nvgLineTo(vg, projectedVolume[5][0], projectedVolume[5][1]);
    nvgStroke(vg);
    
    nvgBeginPath(vg);
    nvgMoveTo(vg, projectedVolume[3][0], projectedVolume[3][1]);
    nvgLineTo(vg, projectedVolume[7][0], projectedVolume[7][1]);
    nvgStroke(vg);
    
    nvgBeginPath(vg);
    nvgMoveTo(vg, projectedVolume[2][0], projectedVolume[2][1]);
    nvgLineTo(vg, projectedVolume[6][0], projectedVolume[6][1]);
    nvgStroke(vg);
}

void
RayCaster::SelectPoints(vector<Point> *projPoints, int width, int height)
{
    // Unselect all
    for (int i = 0; i < projPoints->size(); i++)
    {
        Point &p = (*projPoints)[i];
        // Comment if debug center slice
        //DBG_SelectSlice();
        //
        p.mIsSelected = false;
    }
    
    if (!mSelectionEnabled)
        return;
    
    // Add offsets to keep the selected points exactly inside the drawn borders
    double selection[4] = { mScreenSelection[0]*width + 4.0, (1.0 - mScreenSelection[3])*height + 4.0,
                            mScreenSelection[2]*width + 1.0, (1.0 - mScreenSelection[1])*height + 1.0 };
    
    // Select the points inside the selection rectangle
    for (int i = 0; i < projPoints->size(); i++)
    {
        Point &p = (*projPoints)[i];
        
        if ((p.mX > selection[0]) && (p.mX < selection[2]) &&
            (p.mY > selection[1]) && (p.mY < selection[3]))
        {
            p.mIsSelected = true;
        }
    }
}

void
RayCaster::PropagateSelection(const vector<Point> &projPoints)
{
    // Do nothing if we don't care about selection
    if (!mSelectionEnabled)
        return;
    
    // Apply selection of the selected points to the queue
    long pointId = 0;
    
#if !REVERT_DISPLAY_ORDER
    for (int i = 0; i < mSlices.size(); i++)
#else
    for (int i = mSlices.size() - 1; i >= 0; i--)
#endif
    {
        vector<RayCaster::Point> &vec = mSlices[i];
            
        for (int j = 0; j < vec.size(); j++)
        {
            if (pointId >= projPoints.size())
                // Just in case
                break;
            
            bool selected = projPoints[pointId++].mIsSelected;
                
            Point &p = vec[j];
                
            p.mIsSelected = selected;
        }
    }
}

void
RayCaster::ApplyColorMap(vector<Point> *points)
{
    ColorMap4::CmColor color;
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
        double t = p.mWeight;
        
#if 1
        if (mInvertColormap)
        {
            t = 1.0 - t;
            
            // Saturate less with pow2
            //t = pow(t, 6.0);
            
            // Optim: GOOD (x4)
            t = t*t*t;
            t = t*t;
        }
        
        if (!mInvertColormap)
        {
            // Saturate less with pow2
            //t = pow(t, 2.0);
            
            // Optim: GOOD (x4)
            t = t*t;
        }
#endif
        
        mColorMap->GetColor(t, &color);
        
        unsigned char *col8 = ((unsigned char *)&color);
        p.mR = col8[0];
        p.mG = col8[1];
        p.mB = col8[2];
        p.mA = col8[3];
    }
}

void
RayCaster::UpdateSlicesZ()
{
    // Adjust z for all the history
    for (int i = 0; i < mSlices.size(); i++)
    {
        // Compute time step
        double z = 1.0 - ((double)i)/mSlices.size();
        
        // Center
        z -= 0.5;
        
        // Get the slice
        vector<RayCaster::Point> &points = mSlices[i];
        
        // Set the same time step for all the points of the slice
        for (int j = 0; j < points.size(); j++)
        {
            points[j].mZ = z;
        }
        
#if RENDER_SLICES_TEXTURE
        mGPUSlices[i].SetZ(z);
#endif
    }
}

#if 0 // Shorter implementation
void
RayCaster::DequeToVec(vector<RayCaster::Point> *res,
                      const deque<vector<RayCaster::Point> > &que)
{
    res->clear();
    
#if !REVERT_DISPLAY_ORDER
    for (int i = 0; i < que.size(); i++)
#else
    for (int i = que.size() - 1; i >= 0; i--)
#endif
    {
        res->insert(res->end(), que[i].begin(), que[i].end());
    }
}
#endif

// More verbose implementation to try to optimize
// (but does not optimize a lot)
void
RayCaster::DequeToVec(vector<RayCaster::Point> *res,
                      const deque<vector<RayCaster::Point> > &que)
{
    long numElements = 0;
    for (int i = 0; i < que.size(); i++)
        numElements += que[i].size();
    
    res->resize(numElements);
    
    long elementId = 0;
#if !REVERT_DISPLAY_ORDER
    for (int i = 0; i < que.size(); i++)
#else
    for (int i = que.size() - 1; i >= 0; i--)
#endif
    {
        const vector<RayCaster::Point> &vec = que[i];
        for (int j = 0; j < vec.size(); j++)
        {
            const Point &val = vec[j];
            (*res)[elementId++] = val;
        }
    }
}

int
RayCaster::GetResolution()
{
    int resolution = (1.0 - mQuality)*RESOLUTION_0 + mQuality*RESOLUTION_1;
    
    return resolution;
}

void
RayCaster::ProjectPoint(double projP[3], const double p[3], int width, int height)
{
    glm::mat4 model;
    glm::mat4 proj;
    CameraModelProjMat(width, height, mCamAngle0, mCamAngle1,
                       &model, &proj);

    glm::mat4 modelProjMat = proj*model;
    
    // Matrix transform
    glm::vec4 v;
    v.x = p[0];
    v.y = p[1];
    v.z = p[2];
    v.w = 1.0;
    
    glm::vec4 v4 = modelProjMat*v;
    
    double x = v4.x;
    double y = v4.y;
    double z = v4.z;
    double w = v4.w;
        
#define EPS 1e-8
    if (fabs(w) > EPS)
    {
        // Optim
        double wInv = 1.0/w;
        x *= wInv;
        y *= wInv;
        z *= wInv;
    }
    
    // Do like OpenGL
    x = (x + 1.0)*0.5*width;
    y = (y + 1.0)*0.5*height;
    z = (z + 1.0)*0.5;
    
    // Result
    projP[0] = x;
    projP[1] = y;
    projP[2] = z;
}
