//
//  IsophoteInpaint.cpp
//  BL-Ghost
//
//  Created by Pan on 10/07/18.
//
//

#include <stdlib.h>
#include <memory.h>
#include <limits.h>
#include <math.h>

#include <cmath>

#include "IsophoteInpaint.h"

#define MIN(__x__, __y__) (__x__ < __y__) ? __x__ : __y__

/**
 *  InPaint by isophote continuation
 *
 * @author Xavier Philippeau
 *
 */

#define PRESERVATION 4
#define SAMPLE_REGION_SIZE 24

IsophoteInpaint::Channel::Channel(int w, int h)
{
    mWidth = w;
    mHeight = h;
    
    mData = new BL_FLOAT[h*w];
}

IsophoteInpaint::Channel::~Channel()
{
    delete []mData;
}

BL_FLOAT
IsophoteInpaint::Channel::GetValue(int x, int y)
{
    return mData[x + y*mWidth];
}

void
IsophoteInpaint::Channel::SetValue(int x, int y, BL_FLOAT v)
{
    mData[x + y*mWidth] = v;
}

const BL_FLOAT *
IsophoteInpaint::Channel::GetData()
{
    return mData;
}

//
IsophoteInpaint::IsophoteInpaint(bool processHorizontal, bool processVertical)
{
    // Init for horizontal
    mDx4[0] = 0;
    mDx4[1] = 0;
    mDx4[2] = 0;
    mDx4[3] = 0;
    
    // Init for vertical
    mDy4[0] = 0;
    mDy4[1] = 0;
    mDy4[2] = 0;
    mDy4[3] = 0;
    
    if (processHorizontal)
    {
        mDx4[0] = -1;
        mDx4[1] = 0;
        mDx4[2] = 1;
        mDx4[3] = 0;
    }
    
    if (processVertical)
    {
        mDy4[0] = 0;
        mDy4[1] = -1;
        mDy4[2] = 0;
        mDy4[3] = 1;
    }
    
    //mDxs = NULL;
	//mDys = NULL;
    
    mDistmap = NULL;
    
    mOutput = NULL;
	
    mWidth = 0;
	mHeight = 0;
    
    //mMaskColor[0] = 1.0;
    //mMaskColor[1] = 1.0;
    //mMaskColor[2] = 1.0;
    
    mMaskColor = 1.0;
    
	// isophote preservation factor
	mPreservation = PRESERVATION;
    
    // Niko
    mSampleRegionSize = SAMPLE_REGION_SIZE;
    
    InitSample(mSampleRegionSize);
}

IsophoteInpaint::~IsophoteInpaint() {}

    
void
IsophoteInpaint::InitSample(int nmbsample)
{
    // Initialize neighbours offsets for sampling
    mDxs.resize(nmbsample);
    mDys.resize(nmbsample);
        
    // **** build a spiral curve ****
        
    // directions: Left=(-1,0) Up=(0,-1) Right=(1,0) Down=(0,1)
    int dx[4] = {-1, 0,1,0};
    int dy[4] = { 0,-1,0,1};
    
    int dirIndex = 0;
    int distance = 0;
    int stepToDo = 1;
    int x = 0, y = 0;
		
    while (true)
    {
        // move two times with the same StepCount
        for (int i = 0; i < 2; i++)
        {
            // move
            for (int j = 0; j < stepToDo; j++)
            {
                x += dx[dirIndex];
                y += dy[dirIndex];
                mDxs[distance] = x;
                mDys[distance] = y;
                distance++;
                if (distance >= nmbsample)
                    return;
            }
			
            // turn right
            dirIndex = (dirIndex + 1) % 4;
        }
        // increment StepCount
        stepToDo++;
    }
}
    
void
IsophoteInpaint::Process(const BL_FLOAT *image,
                         const BL_FLOAT *mask,
                         BL_FLOAT **result, int width, int height)
{
    *result = NULL;
    
    // Niko
    mWidth = width;
	mHeight = height;
    
    // GRAYLEVEL IMAGE
    //...
    
#if 0
    // BINARY MASK
    BL_FLOAT *mask = new BL_FLOAT[width*height];
    memset(mask, 0, width*height*sizeof(BL_FLOAT));
    
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            BL_FLOAT rgb = image[x + y*width];
                
            //int gray = (rgb[0]+rgb[1]+rgb[2])/3;
            //input.set(x,y,gray);
                
            if (rgb == mMaskColor)
                mask[x + y*width] = 1.0;
            else
                mask[x + y*width] = 0.0;
        }
    }
#endif
    
    // Inpaint filter
    InpaintLoop(image, mask, width, height);
    
    *result = new BL_FLOAT[width*height];
    memcpy(*result, mOutput->GetData(), width*height*sizeof(BL_FLOAT));
}


// Compute the initial borderline (unmasked pixels close to the mask)
void
IsophoteInpaint::ComputeBorderline(const BL_FLOAT *mask, vector<Point> *borderLine)
{
    for (int y=0; y<mHeight; y++)
    {
        for (int x=0; x<mWidth; x++)
        {
            // for each pixel NOT masked
            BL_FLOAT v = mask[x + y*mWidth];
            if (v > 0.5)
                continue;
                
            // if a neighboor is masked
            // => put the pixel in the borderline list
            for (int k=0; k<4; k++)
            {
                int xk = x+mDx4[k];
                int yk = y+mDy4[k];
                if (xk<0 || xk>=mWidth)
                    continue;
                if (yk<0 || yk>=mHeight)
                    continue;
					
                BL_FLOAT vk = mask[xk + yk*mWidth];
                if (vk > 0.5)
                {
                    Point newP;
                    newP.x = x;
                    newP.y = y;
                    
                    borderLine->push_back(newP);
                    
                    break;
                }
            }
        }
    }
}
    
// iteratively inpaint the image
void
IsophoteInpaint::InpaintLoop(const BL_FLOAT *image, const BL_FLOAT *mask,
                             int width, int height)
{
    mWidth = width;
    mHeight = height;
        
    // initialize output image
    mOutput = new Channel(mWidth, mHeight);
    
    for (int y=0; y<mHeight; y++) {
			for (int x=0; x<mWidth; x++) {
				if (mask[x + y*mWidth] < 0.5)
					mOutput->SetValue(x, y, image[x + y*mWidth]); // known value
				else
					mOutput->SetValue(x, y, -1.0); // unknown value (masked)
			}
		}
        
    // initialize the distance map
	mDistmap = new Channel(mWidth, mHeight);
    for (int y=0; y<mHeight; y++)
        for (int x=0; x<mWidth; x++)
            if (mask[x + y*mWidth] < 0.5)
                mDistmap->SetValue(x, y, 0); // outside the mask -> distance = 0
            else
                mDistmap->SetValue(x, y, INT_MAX); // inside the mask -> distance unknown
        
    // outer borderline
    vector<Point> borderLine;
    IsophoteInpaint::ComputeBorderline(mask, &borderLine);
        
    // iteratively reduce the borderline
    while(!borderLine.empty())
    {
        vector<Point> newBorderLine;
        PropagateBorderLine(borderLine, &newBorderLine);
        
        borderLine = newBorderLine;
    }
}

// inpaint pixels close to the borderline
void
IsophoteInpaint::PropagateBorderLine(const vector<Point> &borderLine,
                                     vector<Point> *newBorderLine)
{
    // for each pixel in the bordeline
    //for (int[] pixel : boderline)
    for (int i = 0; i < borderLine.size(); i++)
    {
        int pixel[2] = { borderLine[i].x, borderLine[i].y };
        
        int x=pixel[0];
        int y=pixel[1];
            
        // distance from the image
        //
        // Niko note: WARN the type !
        int dist = mDistmap->GetValue(x, y);
            
        // explore neighbours, search for uncomputed pixels
        for (int k=0; k<4; k++)
        {
            int xk = x+mDx4[k];
            int yk = y+mDy4[k];
            if (xk<0 || xk>=mWidth)
                continue;
            if (yk<0 || yk>=mHeight)
                continue;
                
            BL_FLOAT vout = mOutput->GetValue(xk,yk);
            if (vout>=0)
                continue; // pixel value is already known.
                
            // compute distance to image
            mDistmap->SetValue(xk, yk, dist+1);
                
            // inpaint this pixel
            BL_FLOAT v = Inpaint(xk, yk);
            if (v<0)
            {
                // should not happen.
                mOutput->SetValue(xk, yk, v);
                continue;
            }
            mOutput->SetValue(xk, yk, v);
                
            // add this pixel to the new borderline
            Point newP;
            newP.x = xk;
            newP.y = yk;
            newBorderLine->push_back(newP);
        }
    }
}
    
// inpaint one pixel
BL_FLOAT
IsophoteInpaint::Inpaint(int x, int y)
{
    BL_FLOAT wsum = 0.0;
    BL_FLOAT vinpaint = 0.0;
        
    int dist = mDistmap->GetValue(x, y);
        
    // sampling pixels in the region
    vector<Point> region;
    for (int k=0; k<mDxs.size(); k++)
    {
        int xk = x+mDxs[k];
        int yk = y+mDys[k];
        if (xk<0 ||xk>=mWidth)
            continue;
        if (yk<0 ||yk>=mHeight)
            continue;
            
        // take only pixels computed in previous loops
        int distk = mDistmap->GetValue(xk, yk);
        if (distk>=dist)
            continue;
        
        Point newP;
        newP.x = xk;
        newP.y = yk;
        region.push_back(newP);
    }
        
    // mean isophote vector of the region
    BL_FLOAT isox = 0, isoy = 0;
    int count=0;
    for (int i = 0; i < region.size(); i++)
    {
        int pixel[2] = { region[i].x, region[i].y };
        int xk = pixel[0];
        int yk = pixel[1];
            
        // isophote direction = normal to the gradient
        BL_FLOAT g[3];
        bool gradOk = Gradient(xk,yk,dist, g);
        if (gradOk)
        {
            isox += -g[1] * g[2];
            isoy += g[0] * g[2];
            count++;
        }
    }
    if (count>0)
    {
        isox/=count;
        isoy/=count;
    }
    
    BL_FLOAT isolength = std::sqrt( isox*isox + isoy*isoy );
        
    // contribution of each pixels in the region
    for (int j = 0; j < region.size(); j++)
    {
        int pixel[2] = { region[j].x, region[j].y };
            
        int xk = pixel[0];
        int yk = pixel[1];
            
        // propagation vector
        int px = x-xk;
        int py = y-yk;
        BL_FLOAT plength = std::sqrt( px*px + py*py );
            
        // Weight of the propagation:
            
        // 1. isophote continuation: cos(isophote,propagation) = normalized dot product ( isophote , propagation )
        BL_FLOAT wisophote = 0;
        if (isolength>0)
        {
	  BL_FLOAT cosangle = std::fabs(isox*px+isoy*py) / (isolength*plength);
            cosangle = MIN(cosangle, 1.0);
            /*
                // linear weight version:
                BL_FLOAT angle = Math.acos(cosangle);
                BL_FLOAT alpha = 1-(angle/Math.PI);
                wisophote = Math.pow(alpha,this.preservation);
            */
            wisophote = std::pow(cosangle, mPreservation);
        }
            
        // 2. spread direction:
        // gradient length = O -> omnidirectionnal
        // gradient length = maxlength -> unidirectionnal
        BL_FLOAT unidir = MIN(isolength/*/255*/,1);
            
        // 3. distance: distance to inpaint pixel
        BL_FLOAT wdist = 1.0 / (1.0 + plength*plength);
            
        // 4. probability: distance to image (unmasked pixel)
        int distk = mDistmap->GetValue(xk, yk);
        BL_FLOAT wproba = 1.0 / (1.0 + distk*distk);
            
        // global weight
        BL_FLOAT w = wdist * wproba * ( unidir*wisophote + (1-unidir)*1 );
            
        vinpaint += w*mOutput->GetValue(xk,yk);
        wsum+=w;
    }
    if (wsum<=0)
        return -1;
    
    vinpaint/=wsum;
        
    if (vinpaint<0)
        vinpaint = 0;
    
    if (vinpaint>1.0)
        vinpaint = 1.0;
    
    return /*(int)*/vinpaint;
}
    
// 8 neightbours gradient
bool
IsophoteInpaint::Gradient(int x, int y, int dist, BL_FLOAT result[3])
{
    // Coordinates of 8 neighbours
    int px = x - 1;  // previous x
    int nx = x + 1;  // next x
    int py = y - 1;  // previous y
    int ny = y + 1;  // next y
        
    // limit to image dimension
    if (px < 0)
        return false;
    if (nx >= mWidth)
        return false;
    if (py < 0)
        return false;
    if (ny >= mHeight)
        return false;
        
    // availability of the 8 neighbours
    // (must be computed in previous loops)
    if (mDistmap->GetValue(px,py)>=dist)
        return false;
    if (mDistmap->GetValue( x,py)>=dist)
        return false;
    if (mDistmap->GetValue(nx,py)>=dist)
        return false;
    if (mDistmap->GetValue(px, y)>=dist)
        return false;
    if (mDistmap->GetValue(nx, y)>=dist)
        return false;
    if (mDistmap->GetValue(px,ny)>=dist)
        return false;
    if (mDistmap->GetValue( x,ny)>=dist)
        return false;
    if (mDistmap->GetValue(nx,ny)>=dist)
        return false;
        
    // Intensity of the 8 neighbours
    BL_FLOAT Ipp = mOutput->GetValue(px,py);
    BL_FLOAT Icp = mOutput->GetValue( x,py);
    BL_FLOAT Inp = mOutput->GetValue(nx,py);
    BL_FLOAT Ipc = mOutput->GetValue(px, y);
    BL_FLOAT Inc = mOutput->GetValue(nx, y);
    BL_FLOAT Ipn = mOutput->GetValue(px,ny);
    BL_FLOAT Icn = mOutput->GetValue( x,ny);
    BL_FLOAT Inn = mOutput->GetValue(nx,ny);
        
    // Local gradient
    BL_FLOAT r2 = 2*std::sqrt(2);
    BL_FLOAT gradx = (Inc-Ipc)/2.0 + (Inn-Ipp)/r2 + (Inp-Ipn)/r2;
    BL_FLOAT grady = (Icn-Icp)/2.0 + (Inn-Ipp)/r2 + (Ipn-Inp)/r2;
    BL_FLOAT norme = std::sqrt(gradx*gradx+grady*grady);
    
    result[0] = gradx;
    result[1] = grady;
    result[2] = norme;
		
    return true;
}

