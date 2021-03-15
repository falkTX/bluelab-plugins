//
//  SparseVolRender.cpp
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#define GLSL_COLORMAP 0
#include <ColorMap4.h>

#include <GraphSwapColor.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <QuadTree.h>

#include "SparseVolRender.h"

//#define POINT_SIZE 4.0

//#define POINT_SIZE 8.0

#define POINT_SIZE 16.0

#define USE_COLORMAP 1 // 0

// Take care of display order for blending
// (otherwise it would make a "hollow" effect)
#define REVERT_DISPLAY_ORDER 1

// Render points instead of grid cells
#define RENDER_SLICE_TEXTURE_SPLAT 1

#define POINT_ALPHA_SPLAT 0.04
#define POINT_SIZE_SPLAT 4.0

#define SLICE_SIZE_ALIGN_VIEW 1
#define POINT_SIZE_SPLAT2 24.0 //48.0 //32.0 //48.0 //32.0 //16.0

#define POINT_ALPHA_SPLAT2 0.02

// Divide resolution factor
#define SLICE_RES_DIV_FACTOR 4.0 //1.0 //4.0

#define USE_TEXTURE_LINEAR 1

#define ADAPTIVE_SLICE_ALPHA 1


#define TEST_BEAUTY_POINT_SELECTION 0

#define DISABLE_DISPLAY_REFRESH_RATE 1

// nvgRect or nvgQuad ?
//
// NOTE: optim x3 with quad rendering instead of rect !
//
// 0 => GOOD !
#define RECT_RENDERING 0

// Try to make very low values transparent (instead of dark and opaque)
// DOESN T WORK...
#define TEST_TRANSPARENT_LOW_VALUES 0 //1

// Choose decimation method
#define DECIMATE_SLICE 0
#define DECIMATE_VOLUME 0
#define DECIMATE_SCREEN_SPACE 1

#define TRANSPARENT_COLORMAP 1

class VolRenderPointMerger : public QuadTree::PointMerger
{
public:
    static VolRenderPointMerger *Get();
    
    virtual ~VolRenderPointMerger();
    
    virtual bool IsMergeable(const SparseVolRender::Point &p0, const SparseVolRender::Point &p1) const;
    
    virtual void Merge(const vector<SparseVolRender::Point> &points, SparseVolRender::Point *res) const;
    
    void SetAlphaCoeff(BL_FLOAT alphaCoeff);
    
protected:
    VolRenderPointMerger();
    
    static VolRenderPointMerger *mInstance;
    
    BL_FLOAT mAlphaCoeff;
};

VolRenderPointMerger *VolRenderPointMerger::mInstance = NULL;

VolRenderPointMerger *
VolRenderPointMerger::Get()
{
    if (mInstance == NULL)
        mInstance = new VolRenderPointMerger();
    
    return mInstance;
}

VolRenderPointMerger::~VolRenderPointMerger() {}

bool
VolRenderPointMerger::IsMergeable(const SparseVolRender::Point &p0,
                                  const SparseVolRender::Point &p1) const
{
    //return true; // TEST
    
    // TODO: include a knob to tweek the threshold
#define THRESHOLD 0.99 //0.25
  if (std::fabs(p0.mWeight - p1.mWeight) < THRESHOLD)
        return true;
    
    return false;
}

void
VolRenderPointMerger::Merge(const vector<SparseVolRender::Point> &points,
                            SparseVolRender::Point *res) const
{
#if 0 // TEST
    if (points.size() == 1)
    // Simply add the point
    {
        const SparseVolRender::Point &p = points[0];
        *res = p;
            
        return;
    }
#endif
    
    res->mX = 0.0;
    res->mY = 0.0;
    res->mZ = 0.0; //
        
    res->mR = 0.0;
    res->mG = 0.0;
    res->mB = 0.0;
    res->mA = 0.0;
        
    res->mWeight = 0.0;
    res->mSize = 0.0;
        
    res->mColorMapId = 0.0;
    
    res->mIsSelected = false;
    
    // Selection flag
    bool selected = false;
    for (int k = 0; k < points.size(); k++)
    {
        const SparseVolRender::Point &p0 = points[k];
        if (p0.mIsSelected)
        {
            selected = true;
            break;
        }
    }
    if (selected)
        res->mIsSelected = selected;

#define INF 1e15
    
    // Bounding box
    BL_FLOAT bboxMin0[2/*3*/] = { INF, INF }; //, INF };
    BL_FLOAT bboxMax0[2/*3*/] = { -INF, -INF }; //, -INF };
        
    BL_FLOAT ratio = 1.0/points.size();
    for (int k = 0; k < points.size(); k++)
    {
        const SparseVolRender::Point &p0 = points[k];
        
        res->mX += p0.mX*ratio;
        res->mY += p0.mY*ratio;
        //res->mZ += p0.mZ*ratio;
            
        res->mR += p0.mR;
        res->mG += p0.mG;
        res->mB += p0.mB;
        res->mA += p0.mA;
            
        res->mWeight += p0.mWeight;
            
        res->mColorMapId += p0.mColorMapId;
        
#if 0
        // Compute the bounding box
        if (p0.mX - p0.mSize/2.0 < bboxMin0[0])
            bboxMin0[0] = p0.mX - p0.mSize/2.0;
        if (p0.mX + p0.mSize/2.0 > bboxMax0[0])
            bboxMax0[0] = p0.mX + p0.mSize/2.0;
            
        if (p0.mY - p0.mSize/2.0 < bboxMin0[1])
            bboxMin0[1] = p0.mY - p0.mSize/2.0;
        if (p0.mY + p0.mSize/2.0 > bboxMax0[1])
            bboxMax0[1] = p0.mY + p0.mSize/2.0;
#endif
        
        // Compute the bounding box
        if (p0.mX  < bboxMin0[0])
            bboxMin0[0] = p0.mX;
        if (p0.mX > bboxMax0[0])
            bboxMax0[0] = p0.mX;
        
        if (p0.mY < bboxMin0[1])
            bboxMin0[1] = p0.mY;
        if (p0.mY > bboxMax0[1])
            bboxMax0[1] = p0.mY;
        
        //if (p0.mZ - p0.mSize/2.0 < bboxMin0[2])
        //    bboxMin0[2] = p0.mZ - p0.mSize/2.0;
        //if (p0.mZ + p0.mSize/2.0 > bboxMax0[2])
        //    bboxMax0[2] = p0.mZ + p0.mSize/2.0;
    }
        
    // Thredshold
    res->mR *= ratio;
    res->mG *= ratio;
    res->mB *= ratio;
        
    res->mA *= ratio;
    
    res->mWeight = 1.0;
        
    res->mColorMapId *= ratio;
        
    // Compute point size depending on points bounding box
    BL_FLOAT bboxSizeX0 = bboxMax0[0] - bboxMin0[0];
    BL_FLOAT bboxSizeY0 = bboxMax0[1] - bboxMin0[1];
    //BL_FLOAT bboxSizeZ0 = bboxMax0[2] - bboxMin0[2];
        
    // TODO: compute the point size better
    //BL_FLOAT bboxSizeX = bboxMax[0] - bboxMin[0];
    //BL_FLOAT bboxSizeY = bboxMax[1] - bboxMin[1];
    //BL_FLOAT bboxSizeZ = bboxMax[2] - bboxMin[2];
    
    // Compute the new point size
    // (algo jam, from the size of a square, and the size of the bbox of the set of points)
    BL_FLOAT sizeX = bboxSizeX0;
    BL_FLOAT sizeY = bboxSizeY0;
    //BL_FLOAT sizeZ = bboxSizeZ0;
            
    res->mSize = std::sqrt(sizeX*sizeX + sizeY*sizeY /*+ sizeZ*sizeZ*/);
    
    //res->mSize *= 100.0;
    
    //res->mA *= 255.0; // TEST
    
    SparseVolRender::ComputePackedPointColor(res, mAlphaCoeff);
}

VolRenderPointMerger::VolRenderPointMerger()
{
    mAlphaCoeff = 1.0;
}

void
VolRenderPointMerger::SetAlphaCoeff(BL_FLOAT alphaCoeff)
{
    mAlphaCoeff = alphaCoeff;
}

//

#if RENDER_SLICES_TEXTURE

SparseVolRender::GPUSlice::GPUSlice()
{
    mVg = NULL;
    
    mNvgImage = -1;
    
    mZ = 0.0;
    
    mImgWidth = 0;
    mImgHeight = 0;
}
    
SparseVolRender::GPUSlice::~GPUSlice()
{
    if (mVg == NULL)
        return;
    
#if 0 // Makes crash
    // TODO: delete when in gl context
    if (mNvgImage != -1)
    {
        nvgDeleteImage(mVg, mNvgImage);
    }
#endif
}

BL_FLOAT
SparseVolRender::GPUSlice::GetZ()
{
    return mZ;
}

void
SparseVolRender::GPUSlice::SetZ(BL_FLOAT z)
{
    mZ = z;
}

int
SparseVolRender::GPUSlice::GetImage()
{
    return mNvgImage;
}

void
SparseVolRender::GPUSlice::CreateImage(NVGcontext *vg,
                                       int width, int height,
                                       const WDL_TypedBuf<unsigned char> &imageDataRGBA)
{
    mVg = vg;
    
    if (mNvgImage != -1)
    {
        nvgDeleteImage(mVg, mNvgImage);
    }
    
    mImgWidth = width;
    mImgHeight = height;
    
    mNvgImage = nvgCreateImageRGBA(mVg,
                                   width, height,
#if !USE_TEXTURE_LINEAR
                                   NVG_IMAGE_NEAREST,
#else
                                   0, // linear
#endif
                                   imageDataRGBA.Get());
}

void
SparseVolRender::GPUSlice::Draw(float corners[4][2], int width, int height,
                                BL_FLOAT alpha)
{
    if (mVg == NULL)
        return;
    
    if (mNvgImage == -1)
        return;
    
#if 0 // TEST
    BL_FLOAT alpha = 1.0; //0.5; //0.02; //1.0; // TEST
    
    //nvgSave(mVg);
    BL_FLOAT paintBounds[4] = { 0.0, 0.0, 1.0, 1.0 };
    NVGpaint imgPaint = nvgImagePattern(mVg,
                                        paintBounds[0]*/*mImgWidth*/width,
                                        paintBounds[1]*/*mImgHeight*/height,
                                        (paintBounds[2] - paintBounds[0])*/*mImgWidth*/width,
                                        (paintBounds[3] - paintBounds[1])*/*mImgHeight*/height,
                                        0.0, mNvgImage, alpha);
    
    RC_FLOAT c0f = corners[0][1];
    RC_FLOAT c1f = corners[1][1];
    RC_FLOAT c2f = corners[2][1];
    RC_FLOAT c3f = corners[3][1];
#if GRAPH_CONTROL_FLIP_Y
    c0f = height - c0f;
    c1f = height - c1f;
    c2f = height - c2f;
    c3f = height - c3f;
#endif
    
    nvgBeginPath(mVg);
    //nvgRect(mVg, ...
    
    nvgMoveTo(mVg, corners[0][0], c0f);
    
    nvgLineTo(mVg, corners[1][0], c1f);
    nvgLineTo(mVg, corners[2][0], c2f);
    nvgLineTo(mVg, corners[3][0], c3f);
    nvgLineTo(mVg, corners[0][0], c0f);
    
    nvgClosePath(mVg);
    
    nvgFillPaint(mVg, imgPaint);
    nvgFill(mVg);
    
    //nvgRestore(mVg);
#endif
    
    nvgSave(mVg);
    
#if ADAPTIVE_SLICE_ALPHA
    nvgFillColor(mVg, nvgRGBA(255, 255, 255, alpha*255));
#endif
    
    //nvgBeginPath(mVg);
    nvgQuad(mVg, corners, mNvgImage);
    //nvgClosePath(mVg);
    
    // Not necessary with quads
    //nvgFill(mVg);
    
    nvgRestore(mVg);
}

#endif

static glm::mat4
CameraMat(int winWidth, int winHeight, float angle0, float angle1)
{
    // Origin (centers better for "scanner")
    //glm::vec3 pos(0.0f, 0.0, -2.0f);
    //glm::vec3 target(0.0f, 0.5f, 0.0f);
    
    // New (centers better with "simple")
    //glm::vec3 pos(0.0f, 0.0, -2.0f);
    //glm::vec3 target(0.0f, 0.25f, 0.0f);
    
    // New 2
    glm::vec3 pos(0.0f, 0.0, -2.0f);
    glm::vec3 target(0.0f, 0.37f, 0.0f);
    
    glm::vec3 up(0.0, 1.0f, 0.0f);

    glm::vec3 lookVec(target.x - pos.x, target.y - pos.y, target.z - pos.z);
    float radius = glm::length(lookVec);
    
    angle1 *= 3.14/180.0;
    float newZ = std::cos(angle1)*radius;
    float newY = std::sin(angle1)*radius;
    
    // Seems to work (no need to touch up vector)
    pos.z = newZ;
    pos.y = newY;
    
    
    glm::mat4 viewMat = glm::lookAt(pos, target, up);
    
    glm::mat4 perspMat = glm::perspective(60.0f, ((float)winWidth)/winHeight, 0.1f, 100.0f);

    glm::mat4 modelMat = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    modelMat = glm::rotate(modelMat, angle0, glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = glm::translate(modelMat, glm::vec3(0.0f, 0.0f, 0.0f));

    glm::mat4 PVMMat = perspMat * viewMat * modelMat;

    return PVMMat;
}

SparseVolRender::SparseVolRender(int numSlices, int numPointsSlice)
{
    mCamAngle0 = 0.0;
    mCamAngle1 = 0.0;
    
    mColorMap = NULL;
    
#if USE_COLORMAP
    SetColorMap(0);
#endif
    
    mInvertColormap = false;
    
    mPointSizeCoeff = 1.0;
    mAlphaCoeff = 1.0;
    
    mNumSlices = numSlices;
    mNumPointsSlice = numPointsSlice;
    
    // Selection
    mSelectionEnabled = false;
    
    mWhitePixImg = -1;
    
#if !DISABLE_DISPLAY_REFRESH_RATE
    mDisplayRefreshRate = 1;
    mRefreshNum = 0;
#endif
    
    mQuality = 1.0;
    
    mQuadTree = NULL;
}

SparseVolRender::~SparseVolRender()
{
    if (mColorMap != NULL)
        delete mColorMap;
    
    if (mQuadTree != NULL)
        delete mQuadTree;
}

void
SparseVolRender::SetNumSlices(int numSlices)
{
    mNumSlices = numSlices;
}

void
SparseVolRender::SetNumPointsSlice(int numPointsSlice)
{
    mNumPointsSlice = numPointsSlice;
}

#if !RENDER_SLICES_TEXTURE // Point rendering
void
SparseVolRender::PreDraw(NVGcontext *vg, int width, int height)
{
    // 3d optimization
    deque<vector<Point> > slices = mSlices;
    
#if DECIMATE_VOLUME
    DecimateVol(&slices);
#endif
    
#if PROFILE_RENDER
    mTimer.Start();
#endif
    
    ProjectPoints(&slices, width, height);
    
    ApplyPointsScale(&slices);
    
#if DECIMATE_SCREEN_SPACE
    DecimateScreenSpace(&slices, width, height);
#endif
    
#define FILL_RECTS 1
    
    nvgSave(vg);
    
    //nvgStrokeWidth(vg, POINT_SIZE);
    
#if 0
    fprintf(stderr, "SparseVolRender:: num points: %ld\n", mPoints.size());
#endif
    
#if !REVERT_DISPLAY_ORDER
    for (int j = 0; j < slices.size(); j++)
#else
    for (int j = slices.size() - 1; j >= 0; j--)
#endif
    {
        const vector<Point> &points = slices[j];
    
        for (int i = 0; i < points.size(); i++)
        {
            const Point &p = points[i];
    
            // Error, since we fill !
            //nvgStrokeWidth(vg, p.mSize*POINT_SIZE);
        
            // OLD
#if 0 // Original: compute the color for each point
            int color[4] = { (int)(p.mR*p.mWeight*255), (int)(p.mG*p.mWeight*255),
                            (int)(p.mB*p.mWeight*255), (int)(p.mA*255) };
        
            for (int j = 0; j < 4; j++)
            {
                if (color[j] > 255)
                    color[j] = 255;
            }
        
            SWAP_COLOR(color);
        
            nvgStrokeColor(vg, nvgRGBA(color[0], color[1], color[2], color[3]));
            nvgFillColor(vg, nvgRGBA(color[0], color[1], color[2], color[3]));
#endif
     
#if 1 // Optim: assign the pre-computed color
            nvgStrokeColor(vg, p.mColor);
            nvgFillColor(vg, p.mColor);
#endif
        
            BL_FLOAT x = p.mX;
            BL_FLOAT y = p.mY;
            
#if 0
            // FIX: when points are very big, they are not centered
            x -= POINT_SIZE*mPointSizeCoeff/2.0;
            y -= POINT_SIZE*mPointSizeCoeff/2.0;
#endif
        
#if TEST_BEAUTY_POINT_SELECTION
            if (p.mIsSelected)
            {
                BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
                yf = height - y;
#endif
                
                nvgBeginPath(vg);
                nvgRect(vg,
                        x, yf,
                        p.mSize*POINT_SIZE*mPointSizeCoeff, p.mSize*POINT_SIZE*mPointSizeCoeff);
                
                nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 255));
                nvgStroke(vg);
            }
#endif
            
#if RECT_RENDERING
            BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
            yf = height - y;
#endif
            
            nvgBeginPath(vg);
        
            //nvgRect(vg, x, y, POINT_SIZE, POINT_SIZE);
            nvgRect(vg,
                    x, yf,
                    p.mSize*POINT_SIZE*mPointSizeCoeff, p.mSize*POINT_SIZE*mPointSizeCoeff);
        
            nvgFill(vg);
#else // Quand rendering
            
            //BL_FLOAT corners[4][2] = { { x - p.mSize*POINT_SIZE*mPointSizeCoeff/2.0, y - p.mSize*POINT_SIZE*mPointSizeCoeff/2.0 },
            //                         { x + p.mSize*POINT_SIZE*mPointSizeCoeff/2.0, y - p.mSize*POINT_SIZE*mPointSizeCoeff/2.0 },
            //                         { x + p.mSize*POINT_SIZE*mPointSizeCoeff/2.0, y + p.mSize*POINT_SIZE*mPointSizeCoeff/2.0 },
            //                         { x - p.mSize*POINT_SIZE*mPointSizeCoeff/2.0, y + p.mSize*POINT_SIZE*mPointSizeCoeff/2.0 } };
            
            BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
            yf = height - yf;
#endif
            
            float corners[4][2] = { {  (float)(x - p.mSize/2.0), (float)(yf - p.mSize/2.0) },
                                     { (float)(x + p.mSize/2.0), (float)(yf - p.mSize/2.0) },
                                     { (float)(x + p.mSize/2.0), (float)(yf + p.mSize/2.0) },
                                     { (float)(x - p.mSize/2.0), (float)(yf + p.mSize/2.0) } };
            
            if (mWhitePixImg < 0)
            {
                unsigned char white[4] = { 255, 255, 255, 255 };
                mWhitePixImg = nvgCreateImageRGBA(vg,
                                                    1, 1,
                                                    NVG_IMAGE_NEAREST,
                                                    white);
            }
            
            nvgQuad(vg, corners, mWhitePixImg);
            
            //nvgFill(vg);
#endif
        }
    }
    
    nvgRestore(vg);
    
    // TODO: extract the drawing of the volume rendering in a separate function
    // (to have a clean PreDraw() )
    // Selection
    DrawSelection(vg, width, height);
    
#if PROFILE_RENDER
    mTimer.Stop();
    
    if (mCount++ > 10)
    {
        long t = mTimer.Get();
        fprintf(stderr, "render: %ld ms\n", t);
        
        mTimer.Reset();
        
        mCount = 0;
    }
#endif
}
#endif


#if RENDER_SLICES_TEXTURE // Slice rendering
void
SparseVolRender::PreDraw(NVGcontext *vg, int width, int height)
{
#if PROFILE_RENDER
    mTimer.Start();
#endif
    
    glm::mat4 transform = CameraMat(width, height, mCamAngle0, mCamAngle1);
    
#define FILL_RECTS 1
    
    nvgSave(vg);
    
#if !REVERT_DISPLAY_ORDER
    for (int j = 0; j < mSlices.size(); j++)
#else
    for (int j = mSlices.size() - 1; j >= 0; j--)
#endif
    {
        GPUSlice &slice = mGPUSlices[j];
        if (slice.GetImage() == -1)
        {
            const vector<Point> &points = mSlices[j];
                
            WDL_TypedBuf<unsigned char> imageData;
            
            BL_FLOAT gridWidth = std::sqrt(mNumPointsSlice);
            int imgSize = std::ceil(gridWidth);
            
#if !RENDER_SLICE_TEXTURE_SPLAT
            PointsToPixels(points, &imageData, imgSize);
#else

#if !SLICE_SIZE_ALIGN_VIEW
            PointsToPixelsSplat(points, &imageData, imgSize, POINT_SIZE*mPointSizeCoeff, POINT_ALPHA_SPLAT);
            slice.CreateImage(vg, imgSize, imgSize, imageData);
#else
            int viewSize = MIN(width, height);
            viewSize /= SLICE_RES_DIV_FACTOR;
            
            PointsToPixelsSplat(points, &imageData, viewSize,
                                POINT_SIZE_SPLAT2*mPointSizeCoeff/SLICE_RES_DIV_FACTOR, POINT_ALPHA_SPLAT2);
            
            slice.CreateImage(vg, viewSize, viewSize, imageData);
#endif
            
#endif
        }
        
        BL_FLOAT sliceZ = slice.GetZ();

        glm::vec4 corners[4];
            
        corners[0].x = -0.5;
        corners[0].y = 0.0;
        corners[0].z = sliceZ;
        corners[0].w = 1.0;
            
        corners[1].x = 0.5;
        corners[1].y = 0.0;
        corners[1].z = sliceZ;
        corners[1].w = 1.0;
            
        corners[2].x = 0.5;
        corners[2].y = 1.0;
        corners[2].z = sliceZ;
        corners[2].w = 1.0;
            
        corners[3].x = -0.5;
        corners[3].y = 1.0;
        corners[3].z = sliceZ;
        corners[3].w = 1.0;
            
    
        BL_FLOAT corners2D[4][2];
            
        for (int k = 0; k < 4; k++)
        {
            corners[k] = transform*corners[k];
            
            BL_FLOAT w = corners[k].w;
    
#define EPS 1e-8
            if (std::fabs(w) > EPS)
            {
                // Optim
                BL_FLOAT wInv = 1.0/w;
                corners[k].x *= wInv;
                corners[k].y *= wInv;
                corners[k].z *= wInv;
            }
                
            BL_FLOAT x = (corners[k].x + 0.5)*width;
            BL_FLOAT y = (corners[k].y + 0.5)*height;
                
            corners2D[k][0] = x;
            corners2D[k][1] = y;
        }

        BL_FLOAT sliceAlpha = 1.0;
#if ADAPTIVE_SLICE_ALPHA
        // A bit arbitrary...
        sliceAlpha = (1.0/mNumSlices)*4.0;
        sliceAlpha = std::sqrt(sliceAlpha); // try to attenuate alpha decrease
#endif
        
        slice.Draw(corners2D, width, height, sliceAlpha);
    }
    
    nvgRestore(vg);
    
    // TODO: extract the drawing of the volume rendering in a separate function
    // (to have a clean PreDraw() )
    // Selection
    DrawSelection(vg, width, height);
    
#if PROFILE_RENDER
    mTimer.Stop();
    
    if (mCount++ > 10)
    {
        long t = mTimer.Get();
        fprintf(stderr, "render: %ld ms\n", t);
        
        mTimer.Reset();
        
        mCount = 0;
    }
#endif
}
#endif

void
SparseVolRender::ClearSlices()
{
    mSlices.clear();
    
#if RENDER_SLICES_TEXTURE
    mGPUSlices.clear();
#endif
}

void
SparseVolRender::SetSlices(const vector<vector<Point> > &slices)
{
    mSlices.clear();
    
    for (int i = 0; i < slices.size(); i++)
        mSlices.push_back(slices[i]);
    
    // Could be improved
    while (mSlices.size() > mNumSlices)
        mSlices.pop_front();
    
#if RENDER_SLICES_TEXTURE
    mGPUSlices.clear();
    
    mGPUSlices.resize(mNumSlices);
#endif
    
    UpdateSlicesZ();
}

void
SparseVolRender::AddSlice(const vector<Point> &points, bool skipDisplay)
{
    vector<Point> points0 = points;
    
    SelectPoints(&points0);
    UpdatePointsSelection(points0);
    
    if (skipDisplay)
        return;
    
#if !DISABLE_DISPLAY_REFRESH_RATE
    if (mRefreshNum++ % mDisplayRefreshRate != 0)
        // Don't add the points this time
        return;
#endif
    
#if DECIMATE_SLICE
    DecimateSlice(&points0, mNumPointsSlice);
#endif
    
#if 0 // Useless, already computed in DecimateSlice()
    ComputePackedPointColors(&points0);
#endif
    
    // NOTE: here, colormap apply doesn't consume a lot of time (~3ms / 15ms total)
    if (mColorMap != NULL)
        ApplyColorMap(&points0);
    
    mSlices.push_back(points0);
    
    while (mSlices.size() > mNumSlices)
        mSlices.pop_front();

#if RENDER_SLICES_TEXTURE
    GPUSlice newGPUSlice;
    mGPUSlices.push_back(newGPUSlice);
    
    while (mGPUSlices.size() > mNumSlices)
        mGPUSlices.pop_front();
#endif
    
    UpdateSlicesZ();
}

void
SparseVolRender::SetCameraAngles(BL_FLOAT angle0, BL_FLOAT angle1)
{
    mCamAngle0 = angle0;
    mCamAngle1 = angle1;
}

void
SparseVolRender::DecimateSlice(vector<SparseVolRender::Point> *points,
                               int maxNumPoints, BL_FLOAT alphaCoeff)
{
    //fprintf(stderr, "num points0: %ld ", points->size());
    
    // Make a grid and sum all the points in each square of the grid
    // in order to have 1 point per square.
    // Also sum some characteristics of the points, and adjust the point size.
    
    // Make the grid
  BL_FLOAT gridWidth = std::sqrt(maxNumPoints);
    
    int gridWidthI = ceil(gridWidth);
    
    vector<vector<vector<SparseVolRender::Point> > > squares;
    squares.resize(gridWidthI);
    for (int i = 0 ; i < gridWidthI; i++)
    {
        squares[i].resize(gridWidthI);
    }
    
    // Compute the bounding box of the points
#define INF 1e15
    BL_FLOAT bboxMin[2] = { INF, INF };
    BL_FLOAT bboxMax[2] = { -INF, -INF };
    for (int i = 0; i < points->size(); i++)
    {
        const SparseVolRender::Point &p = (*points)[i];
        
        if (p.mX < bboxMin[0])
            bboxMin[0] = p.mX;
        if (p.mY < bboxMin[1])
            bboxMin[1] = p.mY;
        
        if (p.mX > bboxMax[0])
            bboxMax[0] = p.mX;
        if (p.mY > bboxMax[1])
            bboxMax[1] = p.mY;
    }
    
    // Doesn't work
#if 0 // Center bounding box to avoid moirage
    // TEST: center the bounding box on 0
    if (bboxMax[0] > -bboxMin[0])
        bboxMin[0] = -bboxMax[0];
    else
        bboxMax[0] = -bboxMin[0];
    
    if (bboxMax[1] > -bboxMin[1])
        bboxMin[1] = -bboxMax[1];
    else
        bboxMax[1] = -bboxMin[1];
#endif
    
    // Not sure it works...
#if 0 // Jittering to avoid moirage
    // Avoid alignment that make patterns on some angles (moirage)
    // To do so, increase randomly the bounding box
    BL_FLOAT bboxWidth0 = bboxMax[0] - bboxMin[0];
    BL_FLOAT bboxHeight0 = bboxMax[1] - bboxMin[1];
    BL_FLOAT bboxMaxDim = (bboxWidth0 > bboxHeight0) ? bboxWidth0 : bboxHeight0;
    
    BL_FLOAT antiAlignAmount = 0.25*bboxMaxDim/gridWidth;
    bboxMin[0] -= antiAlignAmount*((BL_FLOAT)rand())/RAND_MAX;
    bboxMin[1] -= antiAlignAmount*((BL_FLOAT)rand())/RAND_MAX;
    bboxMax[0] += antiAlignAmount*((BL_FLOAT)rand())/RAND_MAX;
    bboxMax[1] += antiAlignAmount*((BL_FLOAT)rand())/RAND_MAX;
#endif
    
    BL_FLOAT bboxWidth = bboxMax[0] - bboxMin[0];
    BL_FLOAT bboxHeight = bboxMax[1] - bboxMin[1];
    
    // Add the points to the squares of the grid
    for (int i = 0; i < points->size(); i++)
    {
        const SparseVolRender::Point &p = (*points)[i];
        
        // Get the point normalized coordinates in the grid
#define EPS 1e-15
        BL_FLOAT normX = 0.0;
        if (bboxWidth > EPS)
        {
            normX = (p.mX - bboxMin[0])/bboxWidth;
            
            // Threshold just in case
            if (normX < 0.0)
                normX = 0.0;
            if (normX > 1.0)
                normX = 1.0;
        }
        
        BL_FLOAT normY = 0.0;
        if (bboxHeight > EPS)
        {
            normY = (p.mY - bboxMin[1])/bboxHeight;
            
            if (normY < 0.0)
                normY = 0.0;
            if (normY > 1.0)
                normY = 1.0;
        }
        
        // Find the square
        int gridX = normX*(gridWidthI - 1);
        int gridY = normY*(gridWidthI - 1);
        
        // Add the point to the square
        squares[gridX][gridY].push_back(p);
        
      // BAD
#if 0 // Works a little but make a lot of blur
        // TEST: add to the neghbors too (to avoid cube pattern)
        if (gridX > 0)
            squares[gridX - 1][gridY].push_back(p);
        if (gridY > 0)
            squares[gridX][gridY - 1].push_back(p);
        
        if (gridX < gridWidthI - 1)
            squares[gridX + 1][gridY].push_back(p);
        if (gridY < gridWidthI - 1)
            squares[gridX][gridY + 1].push_back(p);
#endif
    }
    
    // Empty the result
    points->clear();
    
    // Sum the points in the grid
    for (int j = 0; j < gridWidthI; j++)
    {
        for (int i = 0; i < gridWidthI; i++)
        {
            const vector<SparseVolRender::Point> &square = squares[i][j];
            
            if (square.empty())
                continue;
            
            if (square.size() == 1)
                // Simply add the point
            {
                const SparseVolRender::Point &p = square[0];
                points->push_back(p);
                
                continue;
            }
            
            SparseVolRender::Point newPoint;
            newPoint.mX = 0.0;
            newPoint.mY = 0.0;
            
            newPoint.mR = 0.0;
            newPoint.mG = 0.0;
            newPoint.mB = 0.0;
            newPoint.mA = 0.0;
            
            newPoint.mWeight = 0.0;
            newPoint.mSize = 0.0;
            
            newPoint.mColorMapId = 0.0;
            
            // Compute the sum of the weight, to ponderate
            BL_FLOAT sumWeights = 0.0;
            for (int k = 0; k < square.size(); k++)
            {
                const SparseVolRender::Point &p0 = square[k];
                
                sumWeights += p0.mWeight;
            }
            
            bool selected = false;
            for (int k = 0; k < square.size(); k++)
            {
                const SparseVolRender::Point &p0 = square[k];
                if (p0.mIsSelected)
                {
                    selected = true;
                    break;
                }
            }
            
            // Bounding box
            BL_FLOAT bboxMin0[2] = { INF, INF };
            BL_FLOAT bboxMax0[2] = { -INF, -INF };
            for (int k = 0; k < square.size(); k++)
            {
                const SparseVolRender::Point &p0 = square[k];
            
                BL_FLOAT weight = 0.0;
                if (sumWeights > EPS)
                    weight = p0.mWeight/sumWeights;
                
                newPoint.mX += p0.mX*weight;
                newPoint.mY += p0.mY*weight;
                
                newPoint.mR += p0.mR*weight;
                newPoint.mG += p0.mG*weight;
                newPoint.mB += p0.mB*weight;
                newPoint.mA += p0.mA*weight;
                
                newPoint.mWeight += p0.mWeight;
                
                newPoint.mColorMapId += p0.mColorMapId*weight;
                
                newPoint.mIsSelected = selected;
                
      // BAD: increases too much
#if 0 // Increase the size depending on the number of points
                // NOTE: could be improved, depending on points coordinates
                newPoint.mSize += 1.0;
#endif
                
                // Compute the bounding box
                if (p0.mX - p0.mSize/2.0 < bboxMin0[0])
                    bboxMin0[0] = p0.mX - p0.mSize/2.0;
                if (p0.mY - p0.mSize/2.0 < bboxMin0[1])
                    bboxMin0[1] = p0.mY - p0.mSize/2.0;
                
                if (p0.mX + p0.mSize/2.0 > bboxMax0[0])
                    bboxMax0[0] = p0.mX + p0.mSize/2.0;
                if (p0.mY + p0.mSize/2.0 > bboxMax0[1])
                    bboxMax0[1] = p0.mY + p0.mSize/2.0;
            }
            
#if 0 // No need anymore, since we poderate by intensity
            newPoint.mX /= square.size();
            newPoint.mY /= square.size();
            
            newPoint.mR /= square.size();
            newPoint.mG /= square.size();
            newPoint.mB /= square.size();
            newPoint.mA /= square.size();
#endif
            
            // NOTE: average the intensity to avoid too much saturation
            newPoint.mWeight /= square.size();
            
      // NOTE: with this, the rendering is more beautiful (variable point size)
#if 1 // Compute point size depending on points bounding box
            BL_FLOAT bboxWidth0 = bboxMax0[0] - bboxMin0[0];
            BL_FLOAT bboxHeight0 = bboxMax0[1] - bboxMin0[1];
            
            if ((bboxWidth > 0.0) && (bboxHeight > 0.0))
            {
                // Compute the new point size
                // (algo jam, from the size of a square, and the size of the bbox of the set of points)
                BL_FLOAT sizeX = bboxWidth0/(bboxWidth/*/gridWidthI*/);
                BL_FLOAT sizeY = bboxHeight0/(bboxHeight/*/gridWidthI*/);
            
                //newPoint.mSize = (sizeX > sizeY) ? sizeX : sizeY;
                newPoint.mSize = std::sqrt(sizeX*sizeX + sizeY*sizeY);
            }
#endif
            
            // Compute the packed color for the new point
            ComputePackedPointColor(&newPoint, alphaCoeff);
            
            // Add the new point
            points->push_back(newPoint);
        }
    }
    
    //fprintf(stderr, "num points0: %ld\n", points->size());
}

void
SparseVolRender::DecimateVol(deque<vector<Point> > *slices)
{
    //55296
    long numPointsStart = ComputeNumPoints(*slices);
    
    //#define EPS 1e-8
    if (mQuality < BL_EPS8)
        return;
    
    // NOTE: hard coded 128
    //int numVoxels0 = 1.0/voxelSize;
    int numVoxels0 = 128.0*mQuality;
    if (numVoxels0 <= 0)
        return;
    
    BL_FLOAT voxelSize = 1.0/numVoxels0;
    
    vector<vector<vector<vector<SparseVolRender::Point> > > > voxels;
    voxels.resize(numVoxels0);
    for (int i = 0 ; i < numVoxels0; i++)
    {
        voxels[i].resize(numVoxels0);
        
        for (int j = 0 ; j < numVoxels0; j++)
        {
            voxels[i][j].resize(numVoxels0);
        }
    }
    
    // Flatten the points
    vector<Point> points;
    for (int i = 0; i < slices->size(); i++)
    {
        const vector<Point> &pts = (*slices)[i];
        points.insert(points.end(), pts.begin(), pts.end());
    }
        
    // Compute the bounding box of all the points
#define INF 1e15
    BL_FLOAT bboxMin[3] = { INF, INF, INF };
    BL_FLOAT bboxMax[3] = { -INF, -INF, -INF };
    for (int i = 0; i < points.size(); i++)
    {
        const SparseVolRender::Point &p = points[i];
        
        if (p.mX < bboxMin[0])
            bboxMin[0] = p.mX;
        if (p.mX > bboxMax[0])
            bboxMax[0] = p.mX;
        
        if (p.mY < bboxMin[1])
            bboxMin[1] = p.mY;
        if (p.mY > bboxMax[1])
            bboxMax[1] = p.mY;
        
        if (p.mZ < bboxMin[2])
            bboxMin[2] = p.mZ;
        if (p.mZ > bboxMax[2])
            bboxMax[2] = p.mZ;
    }

    // Add the points to the squares of the grid
    BL_FLOAT bboxSizeX = bboxMax[0] - bboxMin[0];
    BL_FLOAT bboxSizeY = bboxMax[1] - bboxMin[1];
    BL_FLOAT bboxSizeZ = bboxMax[2] - bboxMin[2];
    
    for (int i = 0; i < points.size(); i++)
    {
        const SparseVolRender::Point &p = points[i];
        
        // Get the point normalized coordinates in the grid
        //#define EPS 1e-15
        BL_FLOAT normX = 0.0;
        if (bboxSizeX > BL_EPS)
        {
            normX = (p.mX - bboxMin[0])/bboxSizeX;
            
            // Threshold just in case
            if (normX < 0.0)
                normX = 0.0;
            if (normX > 1.0)
                normX = 1.0;
        }
        
        BL_FLOAT normY = 0.0;
        if (bboxSizeY > BL_EPS)
        {
            normY = (p.mY - bboxMin[1])/bboxSizeY;
            
            if (normY < 0.0)
                normY = 0.0;
            if (normY > 1.0)
                normY = 1.0;
        }
        
        BL_FLOAT normZ = 0.0;
        if (bboxSizeZ > BL_EPS)
        {
            normZ = (p.mZ - bboxMin[2])/bboxSizeZ;
            
            if (normZ < 0.0)
                normZ = 0.0;
            if (normZ > 1.0)
                normZ = 1.0;
        }
        
        // Find the square
        int gridX = normX*(numVoxels0 - 1);
        int gridY = normY*(numVoxels0 - 1);
        int gridZ = normZ*(numVoxels0 - 1);
        
        // Add the point to the square
        voxels[gridX][gridY][gridZ].push_back(p);
    }
    
    //
    // Sum the points in the cube
    //
    vector<Point> newPoints;
    for (int k = 0; k < numVoxels0; k++)
    {
        for (int j = 0; j < numVoxels0; j++)
        {
            for (int i = 0; i < numVoxels0; i++)
            {
            
                const vector<SparseVolRender::Point> &voxel = voxels[i][j][k];
            
                if (voxel.empty())
                    continue;
            
                SparseVolRender::Point newPoint;
                
                //VoxelToPoint(voxel, &newPoint, bboxMin, bboxMax);
                
                VoxelToPoint2(voxel, &newPoint, bboxMin, bboxMax, voxelSize);
                
                // Compute the packed color for the new point
                ComputePackedPointColor(&newPoint, mAlphaCoeff);
                
                // Add the new point
                newPoints.push_back(newPoint);
            }
        }
    }
    
    // Hack: return all the points as contained in a single slice
    slices->clear();
    slices->push_back(newPoints);
    
    long numPointsEnd = ComputeNumPoints(*slices);
    
    fprintf(stderr, "decimate vol: start: %ld  end: %ld\n",
            numPointsStart, numPointsEnd);
}

long
SparseVolRender::ComputeNumPoints(const deque<vector<Point> > &slices)
{
    long numPoints = 0;
    
    for (int i = 0; i < slices.size(); i++)
    {
        const vector<Point> &points = slices[i];
        numPoints += points.size();
    }
    
    return numPoints;
}

// Original code (colors turn dark with low qualities)
void
SparseVolRender::VoxelToPoint(const vector<SparseVolRender::Point> &voxel,
                              SparseVolRender::Point *newPoint,
                              BL_FLOAT bboxMin[3], BL_FLOAT bboxMax[3])
{
    if (voxel.size() == 1)
        // Simply add the point
    {
        const SparseVolRender::Point &p = voxel[0];
        *newPoint = p;
        
        return;
    }
    
    newPoint->mX = 0.0;
    newPoint->mY = 0.0;
    newPoint->mZ = 0.0;
    
    newPoint->mR = 0.0;
    newPoint->mG = 0.0;
    newPoint->mB = 0.0;
    newPoint->mA = 0.0;
    
    newPoint->mWeight = 0.0;
    newPoint->mSize = 0.0;
    
    newPoint->mColorMapId = 0.0;
    
    // Compute the sum of the weight, to ponderate
    BL_FLOAT sumWeights = 0.0;
    for (int l = 0; l < voxel.size(); l++)
    {
        const SparseVolRender::Point &p0 = voxel[l];
        
        sumWeights += p0.mWeight;
    }
    
    bool selected = false;
    for (int l = 0; l < voxel.size(); l++)
    {
        const SparseVolRender::Point &p0 = voxel[l];
        if (p0.mIsSelected)
        {
            selected = true;
            break;
        }
    }
    
    // Bounding box
    BL_FLOAT bboxMin0[3] = { INF, INF, INF };
    BL_FLOAT bboxMax0[3] = { -INF, -INF, -INF };
    for (int l = 0; l < voxel.size(); l++)
    {
        const SparseVolRender::Point &p0 = voxel[l];
        
        BL_FLOAT weight = 0.0;
        if (sumWeights > BL_EPS)
            weight = p0.mWeight/sumWeights;
        
        newPoint->mX += p0.mX*weight;
        newPoint->mY += p0.mY*weight;
        newPoint->mZ += p0.mZ*weight;
        
        newPoint->mR += p0.mR*weight;
        newPoint->mG += p0.mG*weight;
        newPoint->mB += p0.mB*weight;
        newPoint->mA += p0.mA*weight;
        
        newPoint->mWeight += p0.mWeight;
        
        newPoint->mColorMapId += p0.mColorMapId*weight;
        
        newPoint->mIsSelected = selected;
        
        
        // Compute the bounding box
        if (p0.mX - p0.mSize/2.0 < bboxMin0[0])
            bboxMin0[0] = p0.mX - p0.mSize/2.0;
        if (p0.mX + p0.mSize/2.0 > bboxMax0[0])
            bboxMax0[0] = p0.mX + p0.mSize/2.0;
        
        if (p0.mY - p0.mSize/2.0 < bboxMin0[1])
            bboxMin0[1] = p0.mY - p0.mSize/2.0;
        if (p0.mY + p0.mSize/2.0 > bboxMax0[1])
            bboxMax0[1] = p0.mY + p0.mSize/2.0;
        
        if (p0.mZ - p0.mSize/2.0 < bboxMin0[2])
            bboxMin0[2] = p0.mZ - p0.mSize/2.0;
        if (p0.mZ + p0.mSize/2.0 > bboxMax0[2])
            bboxMax0[2] = p0.mZ + p0.mSize/2.0;
    }
    
    // NOTE: average the intensity to avoid too much saturation
    newPoint->mWeight /= voxel.size();
    
    // NOTE: with this, the rendering is more beautiful (variable point size)
    
    // Compute point size depending on points bounding box
    BL_FLOAT bboxSizeX0 = bboxMax0[0] - bboxMin0[0];
    BL_FLOAT bboxSizeY0 = bboxMax0[1] - bboxMin0[1];
    BL_FLOAT bboxSizeZ0 = bboxMax0[2] - bboxMin0[2];
    
    // TODO: compute the point size better
    BL_FLOAT bboxSizeX = bboxMax[0] - bboxMin[0];
    BL_FLOAT bboxSizeY = bboxMax[1] - bboxMin[1];
    BL_FLOAT bboxSizeZ = bboxMax[2] - bboxMin[2];
    
    if ((bboxSizeX > 0.0) && (bboxSizeY > 0.0) && (bboxSizeZ > 0.0))
    {
        // Compute the new point size
        // (algo jam, from the size of a square, and the size of the bbox of the set of points)
        BL_FLOAT sizeX = bboxSizeX0/bboxSizeX;
        BL_FLOAT sizeY = bboxSizeY0/bboxSizeY;
        BL_FLOAT sizeZ = bboxSizeZ0/bboxSizeZ;
        
        newPoint->mSize = std::sqrt(sizeX*sizeX + sizeY*sizeY + sizeZ*sizeZ);
    }
}

void
SparseVolRender::VoxelToPoint2(const vector<SparseVolRender::Point> &voxel,
                               SparseVolRender::Point *newPoint,
                               BL_FLOAT bboxMin[3], BL_FLOAT bboxMax[3],
                               BL_FLOAT voxelSize)
{
    // TODO
    
    if (voxel.size() == 1)
        // Simply add the point
    {
        const SparseVolRender::Point &p = voxel[0];
        *newPoint = p;
        
        return;
    }
    
    newPoint->mX = 0.0;
    newPoint->mY = 0.0;
    newPoint->mZ = 0.0;
    
    newPoint->mR = 0.0;
    newPoint->mG = 0.0;
    newPoint->mB = 0.0;
    newPoint->mA = 0.0;
    
    newPoint->mWeight = 0.0;
    newPoint->mSize = 0.0;
    
    newPoint->mColorMapId = 0.0;
    
    // Compute the sum of the weight, to ponderate
    //BL_FLOAT sumWeights = 0.0;
    //for (int l = 0; l < voxel.size(); l++)
    //{
    //    const SparseVolRender::Point &p0 = voxel[l];
    //
    //    sumWeights += p0.mWeight;
    //}
    
    bool selected = false;
    for (int l = 0; l < voxel.size(); l++)
    {
        const SparseVolRender::Point &p0 = voxel[l];
        if (p0.mIsSelected)
        {
            selected = true;
            break;
        }
    }
    
    // Bounding box
    BL_FLOAT bboxMin0[3] = { INF, INF, INF };
    BL_FLOAT bboxMax0[3] = { -INF, -INF, -INF };
    
    BL_FLOAT ratio = 1.0/voxel.size();
    
    for (int l = 0; l < voxel.size(); l++)
    {
        const SparseVolRender::Point &p0 = voxel[l];
        
        //BL_FLOAT weight = 0.0;
        //if (sumWeights > BL_EPS)
        //    weight = p0.mWeight/sumWeights;
        
        newPoint->mX += p0.mX*ratio; //*weight;
        newPoint->mY += p0.mY*ratio; //*weight;
        newPoint->mZ += p0.mZ*ratio; //*weight;
        
        newPoint->mR += p0.mR; //*weight;
        newPoint->mG += p0.mG; //*weight;
        newPoint->mB += p0.mB; //*weight;
        newPoint->mA += p0.mA; //*p0.mWeight; //*weight;
        
        newPoint->mWeight += p0.mWeight;
        
        newPoint->mColorMapId += p0.mColorMapId; //*weight;
        
        newPoint->mIsSelected = selected;
        
        
        // Compute the bounding box
        if (p0.mX - p0.mSize/2.0 < bboxMin0[0])
            bboxMin0[0] = p0.mX - p0.mSize/2.0;
        if (p0.mX + p0.mSize/2.0 > bboxMax0[0])
            bboxMax0[0] = p0.mX + p0.mSize/2.0;
        
        if (p0.mY - p0.mSize/2.0 < bboxMin0[1])
            bboxMin0[1] = p0.mY - p0.mSize/2.0;
        if (p0.mY + p0.mSize/2.0 > bboxMax0[1])
            bboxMax0[1] = p0.mY + p0.mSize/2.0;
        
        if (p0.mZ - p0.mSize/2.0 < bboxMin0[2])
            bboxMin0[2] = p0.mZ - p0.mSize/2.0;
        if (p0.mZ + p0.mSize/2.0 > bboxMax0[2])
            bboxMax0[2] = p0.mZ + p0.mSize/2.0;
    }
    
    // Thredshold
    newPoint->mR *= ratio;
    newPoint->mG *= ratio;
    newPoint->mB *= ratio;
    
    //if (newPoint->mR > 1.0)
    //    newPoint->mR = 1.0;
    //if (newPoint->mG > 1.0)
    //    newPoint->mG = 1.0;
    //if (newPoint->mB > 1.0)
    //    newPoint->mB = 1.0;
    
    newPoint->mA *= ratio;
    //if (newPoint->mA > 1.0)
    //    newPoint->mA = 1.0;
    
    newPoint->mWeight = 1.0;
    
    //if (newPoint->mWeight > 1.0)
    //    newPoint->mWeight = 1.0;
    
    newPoint->mColorMapId *= ratio;
    
    //if (newPoint->mColorMapId > 1.0)
    //    newPoint->mColorMapId = 1.0;
    
    // NOTE: average the intensity to avoid too much saturation
    //newPoint->mWeight /= voxel.size();
    
    // NOTE: with this, the rendering is more beautiful (variable point size)
    
    // Compute point size depending on points bounding box
    BL_FLOAT bboxSizeX0 = bboxMax0[0] - bboxMin0[0];
    BL_FLOAT bboxSizeY0 = bboxMax0[1] - bboxMin0[1];
    BL_FLOAT bboxSizeZ0 = bboxMax0[2] - bboxMin0[2];
    
    // TODO: compute the point size better
    BL_FLOAT bboxSizeX = bboxMax[0] - bboxMin[0];
    BL_FLOAT bboxSizeY = bboxMax[1] - bboxMin[1];
    BL_FLOAT bboxSizeZ = bboxMax[2] - bboxMin[2];
    
    //if ((bboxSizeX > 0.0) && (bboxSizeY > 0.0) && (bboxSizeZ > 0.0))
    {
        // Compute the new point size
        // (algo jam, from the size of a square, and the size of the bbox of the set of points)
        BL_FLOAT sizeX = bboxSizeX0; ///bboxSizeX;
        BL_FLOAT sizeY = bboxSizeY0; ///bboxSizeY;
        BL_FLOAT sizeZ = bboxSizeZ0; ///bboxSizeZ;
        
        newPoint->mSize = std::sqrt(sizeX*sizeX + sizeY*sizeY + sizeZ*sizeZ);
    }
}

void
SparseVolRender::ProjectPoints(deque<vector<Point> > *slices, int width, int height)
{
    glm::mat4 transform = CameraMat(width, height, mCamAngle0, mCamAngle1);
    
    for (int j = 0; j < slices->size(); j++)
    {
        vector<Point> &points = (*slices)[j];
        
        for (int i = 0; i < points.size(); i++)
        {
            Point &p = points[i];
        
            // Matrix transform
            glm::vec4 v;
            v.x = p.mX;
            v.y = p.mY;
            v.z = p.mZ;
            v.w = 1.0;
        
            glm::vec4 v4 = transform*v;
        
            BL_FLOAT x = v4.x;
            BL_FLOAT y = v4.y;
            BL_FLOAT z = v4.z;
            BL_FLOAT w = v4.w;
        
            //#define EPS 1e-8
            if (std::fabs(w) > BL_EPS8)
            {
                // Optim
                BL_FLOAT wInv = 1.0/w;
                x *= wInv;
                y *= wInv;
                z *= wInv;
            }
        
            // Optim
            x = (x + 0.5)*width;
            y = (y + 0.5)*height;
            
            // Result
            p.mX = x;
            p.mY = y;
            p.mZ = z;
        }
    }
}

void
SparseVolRender::ApplyPointsScale(deque<vector<Point> > *slices)
{
    for (int j = 0; j < slices->size(); j++)
    {
        vector<Point> &points = (*slices)[j];
        
        for (int i = 0; i < points.size(); i++)
        {
            Point &p = points[i];
            
            p.mSize *= POINT_SIZE*mPointSizeCoeff;
        }
    }
}

void
SparseVolRender::DecimateScreenSpace(deque<vector<Point> > *slices, int width, int height)
{
    // TODO: do not use lazy evaluation
    // (would make plugin slowdown during playback)
    //
    // TODO instead: make the tree in the SparseVolRender constructor.
    //
    if (mQuadTree == NULL)
    {
        int maxDim = (width > height) ? width : height;
        
        //int resolution[2] = { maxDim, maxDim };
        //int resolution[2] = { 512, 512 };
        int resolution[2] = { 128, 128 };
        
        BL_FLOAT bbox[2][2] = { { 0, 0 }, { (BL_FLOAT)maxDim, (BL_FLOAT)maxDim } };
        
        mQuadTree = QuadTree::BuildFromBottom(bbox, resolution);
    }
    
    // Convert deque to vector
    vector<Point> points;
    for (int i = slices->size() - 1; i >= 0; i--)
        points.insert(points.end(), (*slices)[i].begin(), (*slices)[i].end());
    
    int numPointsBegin = points.size();
    
    // Process points
    QuadTree::InsertPoints(mQuadTree, points);

    VolRenderPointMerger *pointMerger = VolRenderPointMerger::Get();
    pointMerger->SetAlphaCoeff(mAlphaCoeff);
    
    QuadTree::MergePoints(mQuadTree, *pointMerger);
    
    points.clear();
    QuadTree::GetPoints(mQuadTree, &points);
    
    mQuadTree->Clear();
    
    // Convert vector to deque
    slices->clear();
    slices->push_back(points);
    
    int numPointsEnd = points.size();
    
    fprintf(stderr, "screen space: start: %d end: %d\n",
            numPointsBegin, numPointsEnd);
}

void
SparseVolRender::ComputePackedPointColor(SparseVolRender::Point *p, BL_FLOAT alphaCoeff)
{
    BL_FLOAT intensity = p->mWeight;
    
#if USE_COLORMAP // HACK
    intensity = 0.005;
#endif
    
#if !TEST_BEAUTY_POINT_SELECTION
    // Hack (another...)
    if (p->mIsSelected)
    {
#define SELECTED_COEFF 10.0
        
        //p->mR *= SELECTED_COEFF;
        //p->mG *= SELECTED_COEFF;
        //p->mB *= SELECTED_COEFF;
        
        //p->mA *= SELECTED_COEFF;
        p->mA = 255.0; //1.0;
    }
#endif
    
    // Optim: pre-compute nvg color
    int color[4] = { (int)(p->mR*intensity*255), (int)(p->mG*intensity*255),
        (int)(p->mB*intensity*255), (int)(p->mA*alphaCoeff*255) };
    
    for (int j = 0; j < 4; j++)
    {
        if (color[j] > 255)
            color[j] = 255;
    }
    
    SWAP_COLOR(color);
    
    // Try to make very low values transparent (instead of dark and opaque)
    // DOESN T WORK...
#if TEST_TRANSPARENT_LOW_VALUES
    color[3] *= p->mWeight;
#endif
    
    p->mColor = nvgRGBA(color[0], color[1], color[2], color[3]);
}

void
SparseVolRender::ComputePackedPointColors(vector<SparseVolRender::Point> *points)
{
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        ComputePackedPointColor(&p, mAlphaCoeff);
    }
}

void
SparseVolRender::RefreshPackedPointColors()
{
    for (int i = 0; i < mSlices.size(); i++)
    {
        vector<Point> &points = mSlices[i];
        ComputePackedPointColors(&points);
    }
}
void
SparseVolRender::SetColorMap(int colormapId)
{
    bool glsl = false; // #bl-iplug2
    switch(colormapId)
    {
        case 0:
        {
            if (mColorMap != NULL)
                delete mColorMap;
    
            // Blue and dark pink
            mColorMap = new ColorMap4(glsl);
            
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
            mColorMap = new ColorMap4(glsl);
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
            mColorMap = new ColorMap4(glsl);
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
    
    // Refresh the color of the points
    ApplyColorMapSlices(&mSlices);
    
#if 0
    // Sky (ice)
    mColorMap = new ColorMap4();
    mColorMap->AddColor(4, 6, 19, 255, 0.0);
    
    mColorMap->AddColor(58, 61, 126, 255, 0.25);
    mColorMap->AddColor(67, 126, 184, 255, 0.5);
    
    mColorMap->AddColor(73, 173, 208, 255, 0.75);
    
    mColorMap->AddColor(232, 251, 252, 255, 1.0);
#endif
}

void
SparseVolRender::SetInvertColormap(bool flag)
{
    mInvertColormap = flag;
    
    ApplyColorMapSlices(&mSlices);
}

void
SparseVolRender::SetColormapRange(BL_FLOAT range)
{
    if (mColorMap != NULL)
    {
        mColorMap->SetRange(range);
        
        ApplyColorMapSlices(&mSlices);
    }
}

void
SparseVolRender::SetColormapContrast(BL_FLOAT contrast)
{
    if (mColorMap != NULL)
    {
        mColorMap->SetContrast(contrast);
        
        ApplyColorMapSlices(&mSlices);
    }
}

void
SparseVolRender::SetPointSizeCoeff(BL_FLOAT coeff)
{
    mPointSizeCoeff = coeff;
}

void
SparseVolRender::SetAlphaCoeff(BL_FLOAT coeff)
{
    mAlphaCoeff = coeff;
    
    RefreshPackedPointColors();
}

void
SparseVolRender::SetSelection(BL_FLOAT selection[4])
{
    for (int i = 0; i < 4; i++)
        mSelection[i] = selection[i];
    
    // Re-order the selection
    if (mSelection[2] < mSelection[0])
    {
        BL_FLOAT tmp = mSelection[2];
        mSelection[2] = mSelection[0];
        mSelection[0] = tmp;
    }
    
    if (mSelection[3] < mSelection[1])
    {
        BL_FLOAT tmp = mSelection[3];
        mSelection[3] = mSelection[1];
        mSelection[1] = tmp;
    }
    
    mSelectionEnabled = true;
}

void
SparseVolRender::DisableSelection()
{
    mSelectionEnabled = false;
}

void
SparseVolRender::GetPointsSelection(vector<bool> *pointFlags)
{
    *pointFlags = mPointsSelection;
}

void
SparseVolRender::SetDisplayRefreshRate(int displayRefreshRate)
{
#if !DISABLE_DISPLAY_REFRESH_RATE
    mDisplayRefreshRate = displayRefreshRate;
#endif
}

void
SparseVolRender::SetQuality(BL_FLOAT quality)
{
    mQuality = quality;
}

void
SparseVolRender::DrawSelection(NVGcontext *vg, int width, int height)
{
    if (!mSelectionEnabled)
        return;
    
    BL_FLOAT strokeWidths[2] = { 3.0, 2.0 };
    
    // Two colors, for drawing two times, for overlay
    int colors[2][4] = { { 64, 64, 64, 255 }, { 255, 255, 255, 255 } };
    
    for (int i = 0; i < 2; i++)
    {
        nvgStrokeWidth(vg, strokeWidths[i]);
        
        SWAP_COLOR(colors[i]);
        nvgStrokeColor(vg, nvgRGBA(colors[i][0], colors[i][1], colors[i][2], colors[i][3]));
        
        // Draw the circle
        nvgBeginPath(vg);
        
        BL_GUI_FLOAT s1f = (1.0 - mSelection[1])*height;
        BL_GUI_FLOAT s3f = (1.0 - mSelection[3])*height;
#if GRAPH_CONTROL_FLIP_Y
        s1f = height - s1f;
        s3f = height - s3f;
#endif
        
        // Draw the line
        nvgMoveTo(vg, mSelection[0]*width, s1f);
        
        nvgLineTo(vg, mSelection[2]*width, s1f);
        nvgLineTo(vg, mSelection[2]*width, s3f);
        nvgLineTo(vg, mSelection[0]*width, s3f);
        nvgLineTo(vg, mSelection[0]*width, s1f);
        
        nvgStroke(vg);
    }
}

void
SparseVolRender::SelectPoints(vector<Point> *points)
{
    // Unselect all
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        p.mIsSelected = false;
    }
    
    // Select the points that are inside the selection rect
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
        if ((p.mX > mSelection[0] - 0.5) && (p.mX < mSelection[2] - 0.5) &&
            (p.mY < 1.0 - mSelection[1]) && (p.mY > 1.0 - mSelection[3]))
            p.mIsSelected = true;
    }
}

void
SparseVolRender::UpdatePointsSelection(const vector<Point> &points)
{
    mPointsSelection.resize(points.size());
    
    for (int i = 0; i < points.size(); i++)
    {
        const Point &p = points[i];
        
        bool selectedFlag = p.mIsSelected;
        
        mPointsSelection[i] = selectedFlag;
    }
}

void
SparseVolRender::ApplyColorMap(vector<Point> *points)
{
    ColorMap4::CmColor color;
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
        BL_FLOAT t = p.mColorMapId;
        
        if (mInvertColormap)
        {
            t = 1.0 - t;
            
            // Saturate less with pow2
            t = std::pow(t, 6.0);
        }
        
        if (!mInvertColormap)
        {
            // Saturate less with pow2
	  t = std::pow(t, 2.0);
        }
        
        mColorMap->GetColor(t, &color);
        
        unsigned char *col8 = ((unsigned char *)&color);
        p.mR = col8[0];
        p.mG = col8[1];
        p.mB = col8[2];
        
        p.mA = ((BL_FLOAT)col8[3])/255.0;
        
        ComputePackedPointColor(&p, mAlphaCoeff);
    }
}

void
SparseVolRender::ApplyColorMapSlices(deque<vector<Point> > *slices)
{
    for (int i = 0; i < slices->size(); i++)
    {
        vector<Point> &points = (*slices)[i];
        
        ApplyColorMap(&points);
    }
}

void
SparseVolRender::UpdateSlicesZ()
{
    // Adjust z for all the history
    for (int i = 0; i < mSlices.size(); i++)
    {
        // Compute time step
        BL_FLOAT z = 1.0 - ((BL_FLOAT)i)/mSlices.size();
        
        // Center
        z -= 0.5;
        
        // Get the slice
        vector<SparseVolRender::Point> &points = mSlices[i];
        
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
    
#if RENDER_SLICES_TEXTURE
void
SparseVolRender::PointsToPixels(const vector<Point> &points,
                                WDL_TypedBuf<unsigned char> *pixels,
                                int imgSize)
{
    pixels->Resize(imgSize*imgSize*4);
    memset(pixels->Get(), 0, imgSize*imgSize*4);
    
    // NOTE: some code is redundant with DecimateSlice
    
    // Compute the bounding box of the points
#define INF 1e15
    BL_FLOAT bboxMin[2] = { INF, INF };
    BL_FLOAT bboxMax[2] = { -INF, -INF };
    for (int i = 0; i < points.size(); i++)
    {
        const SparseVolRender::Point &p = points[i];
        
        if (p.mX < bboxMin[0])
            bboxMin[0] = p.mX;
        if (p.mY < bboxMin[1])
            bboxMin[1] = p.mY;
        
        if (p.mX > bboxMax[0])
            bboxMax[0] = p.mX;
        if (p.mY > bboxMax[1])
            bboxMax[1] = p.mY;
    }
    
    BL_FLOAT bboxWidth = bboxMax[0] - bboxMin[0];
    BL_FLOAT bboxHeight = bboxMax[1] - bboxMin[1];
    
    
    for (int i = 0; i < points.size(); i++)
    {
        const Point &p = points[i];
        
        // 1 - Find the grid cell
        
        // Get the point normalized coordinates in the grid
        //#define EPS 1e-15
        BL_FLOAT normX = 0.0;
        if (bboxWidth > BL_EPS)
        {
            normX = (p.mX - bboxMin[0])/bboxWidth;
            
            // Threshold just in case
            if (normX < 0.0)
                normX = 0.0;
            if (normX > 1.0)
                normX = 1.0;
        }
        
        BL_FLOAT normY = 0.0;
        if (bboxHeight > BL_EPS)
        {
            normY = (p.mY - bboxMin[1])/bboxHeight;
            
            if (normY < 0.0)
                normY = 0.0;
            if (normY > 1.0)
                normY = 1.0;
        }
        
        // TEST: try to match with point rendering
        //normX /= 2.0;
        //normX += 0.25;
        
        // Find the square
        int gridX = normX*(imgSize - 1);
        int gridY = normY*(imgSize - 1);
        
        // 2 - Fill the texel
        const NVGcolor &col = p.mColor;
        
        int pixelIndex = (gridX + gridY*imgSize)*4;
        unsigned char *pixel = &pixels->Get()[pixelIndex];
        
        for (int k = 0; k < 4; k++)
        {
            pixel[k] = col.rgba[k]*255;
        }
        
        // TEST
        //pixel[3] = 64;
        
        //SWAP_COLOR(pixel);
    }
}

void
SparseVolRender::PointsToPixelsSplat(const vector<Point> &points,
                                     WDL_TypedBuf<unsigned char> *pixels,
                                     int imgSize, BL_FLOAT pointSize,
                                     BL_FLOAT alpha)
{
    pixels->Resize(imgSize*imgSize*4);
    memset(pixels->Get(), 0, imgSize*imgSize*4);
    
    // NOTE: some code is redundant with DecimateSlice
    
    // Compute the bounding box of the points
#define INF 1e15
    BL_FLOAT bboxMin[2] = { INF, INF };
    BL_FLOAT bboxMax[2] = { -INF, -INF };
    for (int i = 0; i < points.size(); i++)
    {
        const SparseVolRender::Point &p = points[i];
        
        if (p.mX < bboxMin[0])
            bboxMin[0] = p.mX;
        if (p.mY < bboxMin[1])
            bboxMin[1] = p.mY;
        
        if (p.mX > bboxMax[0])
            bboxMax[0] = p.mX;
        if (p.mY > bboxMax[1])
            bboxMax[1] = p.mY;
    }
    
    BL_FLOAT bboxWidth = bboxMax[0] - bboxMin[0];
    BL_FLOAT bboxHeight = bboxMax[1] - bboxMin[1];
    
    
    for (int i = 0; i < points.size(); i++)
    {
        const Point &p = points[i];
        
        // 1 - Find the grid cell
        
        // Get the point normalized coordinates in the grid
        //#define EPS 1e-15
        BL_FLOAT normX = 0.0;
        if (bboxWidth > BL_EPS)
        {
            normX = (p.mX - bboxMin[0])/bboxWidth;
            
            // Threshold just in case
            if (normX < 0.0)
                normX = 0.0;
            if (normX > 1.0)
                normX = 1.0;
        }
        
        BL_FLOAT normY = 0.0;
        if (bboxHeight > BL_EPS)
        {
            normY = (p.mY - bboxMin[1])/bboxHeight;
            
            if (normY < 0.0)
                normY = 0.0;
            if (normY > 1.0)
                normY = 1.0;
        }
      
        // TEST: try to match with point rendering
        //normX /= 2.0;
        //normX += 0.25;
        
#if 0
        // TEST
        normX = (normX - 0.5)/4.0 + 0.5;
        normY = (normY - 0.5)/2.0 + 0.5;
#endif
        
        // Find the point coordinate, in pixels
        //
        // NOTE: adjust with pointsSize, to be able to fit all the point rectangle
        // inside the slice.
        // (otherwise, that would lead to a neat cut at the boudaries (left) )
        int gridX = normX*(imgSize - 1 - pointSize) + pointSize/2.0;
        int gridY = normY*(imgSize - 1 - pointSize) + pointSize/2.0;
        
        // 2 - Fill the texel
        //const NVGcolor &col = p.mColor;
        
        // Overwrite alpha
        NVGcolor col = p.mColor;
        col.rgba[3] = alpha;
        
        BL_FLOAT pSize = pointSize*p.mSize;
        for (int j = -pSize/2; j < pSize/2; j++)
        {
            int x = gridX + j;
            if (x < 0)
                continue;
            if (x > imgSize)
                continue;
            
            for (int k = -pSize/2; k < pSize/2; k++)
            {
                int y = gridY + k;
                if (y < 0)
                    continue;
                if (y > imgSize)
                    continue;
                
                int pixelIndex = (x + y*imgSize)*4;
                unsigned char *pixel = &pixels->Get()[pixelIndex];
                
                // Rigourously implement OVER operator:
                //
                // See: https://en.wikipedia.org/wiki/Alpha_compositing
                //
                
                BL_FLOAT denom = col.rgba[3] + (pixel[3]/255.0)*(1.0 - col.rgba[3]);
                for (int c = 0; c < 3; c++)
                {
                    BL_FLOAT cc = ((col.rgba[c]*255)*col.rgba[3] +
                                 pixel[c]*(pixel[3]/255.0)*(1.0 - col.rgba[3]))/denom;
                    
                    if (cc > 255.0)
                        cc = 255.0;
                    
                    pixel[c] = cc;
                }
                
                pixel[3] = denom*255;
            }
        }
        
        
        //SWAP_COLOR(pixel);
    }
}
#endif

#endif // IGRAPHICS_NANOVG
