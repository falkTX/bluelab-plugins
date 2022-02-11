//
//  SparseVolRender.cpp
//  BL-StereoWidth
//
//  Created by applematuer on 10/13/18.
//
//

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#define GLSL_COLORMAP 0
#include <ColorMap4.h>

#include "SparseVolRender.h"

//#define POINT_SIZE 4.0

//#define POINT_SIZE 8.0

#define POINT_SIZE 16.0

#define USE_COLORMAP 1 // 0

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
    float newZ = cos(angle1)*radius;
    float newY = sin(angle1)*radius;
    
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
    
    mNumSlices = numSlices;
    mNumPointsSlice = numPointsSlice;
}

SparseVolRender::~SparseVolRender()
{
    if (mColorMap != NULL)
        delete mColorMap;
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

void
SparseVolRender::PreDraw(NVGcontext *vg, int width, int height)
{
    glm::mat4 transform = CameraMat(width, height, mCamAngle0, mCamAngle1);
    
#define FILL_RECTS 1
    
    nvgSave(vg);
    
    //nvgStrokeWidth(vg, POINT_SIZE);
    
#if 0
    fprintf(stderr, "SparseVolRender:: num points: %ld\n", mPoints.size());
#endif
    
    for (int j = 0; j < mSlices.size(); j++)
    {
        const vector<Point> &points = mSlices[j];
    
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
        
            // Matrix transform
            glm::vec4 v;
            v.x = p.mX;
            v.y = p.mY;
            v.z = p.mZ;
            v.w = 1.0;
        
            glm::vec4 v4 = transform*v;
        
            double x = v4.x;
            double y = v4.y;
            double z = v4.z;
            double w = v4.w;
        
#define EPS 1e-8
            if (fabs(w) > EPS)
            {
                x /= w;
                y /= w;
                z /= w;
            }
                
            x = x*width + width/2.0;
            y = y*height + height/2.0;
        
#if 0
            // FIX: when points are very big, they are not centered
            x -= POINT_SIZE/2.0;
            y -= POINT_SIZE/2.0;
#endif
        
            nvgBeginPath(vg);
        
            //nvgRect(vg, x, y, POINT_SIZE, POINT_SIZE);
            nvgRect(vg, x, y, p.mSize*POINT_SIZE, p.mSize*POINT_SIZE);
        
            nvgFill(vg);
        }
    }
    
    nvgRestore(vg);
}

void
SparseVolRender::ClearSlices()
{
    mSlices.clear();
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
    
    UpdateSlicesZ();
}

void
SparseVolRender::AddSlice(const vector<Point> &points)
{
    vector<Point> points0 = points;
    
    DecimateSlice(&points0, mNumPointsSlice);
    
    ComputePackedPointColors(&points0);
    
    // NOTE: here, colormap apply doesn't consume a lot of time (~3ms / 15ms total)
    if (mColorMap != NULL)
        ApplyColorMap(&points0);
    
    mSlices.push_back(points0);
    
    while (mSlices.size() > mNumSlices)
        mSlices.pop_front();

    UpdateSlicesZ();
}

void
SparseVolRender::SetCameraAngles(double angle0, double angle1)
{
    mCamAngle0 = angle0;
    mCamAngle1 = angle1;
}

void
SparseVolRender::DecimateSlice(vector<SparseVolRender::Point> *points, int maxNumPoints)
{
    //fprintf(stderr, "num points0: %ld ", points->size());
    
    // Make a grid and sum all the points in each square of the grid
    // in order to have 1 point per square.
    // Also sum some characteristics of the points, and adjust the point size.
    
    // Make the grid
    double gridWidth = sqrt(maxNumPoints);
    
    int gridWidthI = ceil(gridWidth);
    
    vector<vector<vector<SparseVolRender::Point> > > squares;
    squares.resize(gridWidthI);
    for (int i = 0 ; i < gridWidthI; i++)
    {
        squares[i].resize(gridWidthI);
    }
    
    // Compute the bounding box of the points
#define INF 1e15
    double bboxMin[2] = { INF, INF };
    double bboxMax[2] = { -INF, -INF };
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
    double bboxWidth0 = bboxMax[0] - bboxMin[0];
    double bboxHeight0 = bboxMax[1] - bboxMin[1];
    double bboxMaxDim = (bboxWidth0 > bboxHeight0) ? bboxWidth0 : bboxHeight0;
    
    double antiAlignAmount = 0.25*bboxMaxDim/gridWidth;
    bboxMin[0] -= antiAlignAmount*((double)rand())/RAND_MAX;
    bboxMin[1] -= antiAlignAmount*((double)rand())/RAND_MAX;
    bboxMax[0] += antiAlignAmount*((double)rand())/RAND_MAX;
    bboxMax[1] += antiAlignAmount*((double)rand())/RAND_MAX;
#endif
    
    double bboxWidth = bboxMax[0] - bboxMin[0];
    double bboxHeight = bboxMax[1] - bboxMin[1];
    
    // Add the points to the squares of the grid
    for (int i = 0; i < points->size(); i++)
    {
        const SparseVolRender::Point &p = (*points)[i];
        
        // Get the point normalized coordinates in the grid
#define EPS 1e-15
        double normX = 0.0;
        if (bboxWidth > EPS)
        {
            normX = (p.mX - bboxMin[0])/bboxWidth;
            
            // Threshold just in case
            if (normX < 0.0)
                normX = 0.0;
            if (normX > 1.0)
                normX = 1.0;
        }
        
        double normY = 0.0;
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
            
            // Compute the sum of the weighrs, to ponderate
            double sumWeights = 0.0;
            for (int k = 0; k < square.size(); k++)
            {
                const SparseVolRender::Point &p0 = square[k];
                
                sumWeights += p0.mWeight;
            }
            
            // Bounding box
            double bboxMin0[2] = { INF, INF };
            double bboxMax0[2] = { -INF, -INF };
            for (int k = 0; k < square.size(); k++)
            {
                const SparseVolRender::Point &p0 = square[k];
            
                double weight = 0.0;
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
            double bboxWidth0 = bboxMax0[0] - bboxMin0[0];
            double bboxHeight0 = bboxMax0[1] - bboxMin0[1];
            
            // Compute the new point size
            // (algo jam, from the size of a square, and the size of the bbox of the set of points)
            double sizeX = bboxWidth0/(bboxWidth/*/gridWidthI*/);
            double sizeY = bboxHeight0/(bboxHeight/*/gridWidthI*/);
            
            //newPoint.mSize = (sizeX > sizeY) ? sizeX : sizeY;
            newPoint.mSize = sqrt(sizeX*sizeX + sizeY*sizeY);
#endif
            
            // Compute the packed color for the new point
            ComputePackedPointColor(&newPoint);
            
            // Add the new point
            points->push_back(newPoint);
        }
    }
    
    //fprintf(stderr, "num points0: %ld\n", points->size());
}

void
SparseVolRender::ComputePackedPointColor(SparseVolRender::Point *p)
{
    double intensity = p->mWeight;
    
#if USE_COLORMAP // HACK
    intensity = 0.005;
#endif
    
    // Optim: pre-compute nvg color
    int color[4] = { (int)(p->mR*intensity*255), (int)(p->mG*intensity*255),
        (int)(p->mB*intensity*255), (int)(p->mA*255) };
    for (int j = 0; j < 4; j++)
    {
        if (color[j] > 255)
            color[j] = 255;
    }
    
    SWAP_COLOR(color);
    
    p->mColor = nvgRGBA(color[0], color[1], color[2], color[3]);
}

void
SparseVolRender::ComputePackedPointColors(vector<SparseVolRender::Point> *points)
{
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        ComputePackedPointColor(&p);
    }
}

void
SparseVolRender::SetColorMap(int colormapId)
{
    switch(colormapId)
    {
        case 0:
        {
            if (mColorMap != NULL)
                delete mColorMap;
    
            // Blue and dark pink
            mColorMap = new ColorMap4();
            mColorMap->AddColor(0, 0, 0, 255, 0.0);
            
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
            mColorMap = new ColorMap4();
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
            mColorMap = new ColorMap4();
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
SparseVolRender::ApplyColorMap(vector<Point> *points)
{
    ColorMap4::CmColor color;
    for (int i = 0; i < points->size(); i++)
    {
        Point &p = (*points)[i];
        
        double t = p.mColorMapId;
        
        if (mInvertColormap)
        {
            t = 1.0 - t;
            
            // Saturate less with pow2
            t = pow(t, 6.0);
        }
        
        if (!mInvertColormap)
        {
            // Saturate less with pow2
            t = pow(t, 2.0);
        }
        
        mColorMap->GetColor(t, &color);
        
        unsigned char *col8 = ((unsigned char *)&color);
        p.mR = col8[0];
        p.mG = col8[1];
        p.mB = col8[2];
        
        ComputePackedPointColor(&p);
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
        double z = 1.0 - ((double)i)/mSlices.size();
        
        // Center
        z -= 0.5;
        
        // Get the slice
        vector<SparseVolRender::Point> &points = mSlices[i];
        
        // Set the same time step for all the points of the slice
        for (int j = 0; j < points.size(); j++)
        {
            points[j].mZ = z;
        }
    }
}
