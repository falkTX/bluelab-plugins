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
//  IsophoteInpaint.h
//  BL-Ghost
//
//  Created by Pan on 10/07/18.
//
//

#ifndef __BL_Ghost__IsophoteInpaint__
#define __BL_Ghost__IsophoteInpaint__

#include <vector>
using namespace std;

#include <BLTypes.h>

class IsophoteInpaint
{
public:
    IsophoteInpaint(bool processHorizontal = true, bool processVertical = true);
    
    virtual ~IsophoteInpaint();
    
    // Gray level image
    void Process(const BL_FLOAT *image,
                 const BL_FLOAT *mask,
                 BL_FLOAT **result, int width, int height);
    
protected:
    typedef struct
    {
        int x;
        int y;
    } Point;
    
    void InitSample(int nmbsample);
    
    void InpaintLoop(const BL_FLOAT *image, const BL_FLOAT *mask, int width, int height);
    
    void ComputeBorderline(const BL_FLOAT *mask, vector<Point> *borderLine);

    void PropagateBorderLine(const vector<Point> &borderLine, vector<Point> *newBorderLine);
    
    BL_FLOAT Inpaint(int x, int y);
    
    // return true on success
    bool Gradient(int x, int y, int dist, BL_FLOAT result[3]);

    
    // Temporary workspace
	class Channel
    {
    public:
        Channel(int w,int h);
		
        virtual ~Channel();
        
        BL_FLOAT GetValue(int x, int y);
		
        void SetValue(int x, int y, BL_FLOAT v);
        
        const BL_FLOAT *GetData();
        
    private:
        BL_FLOAT *mData;
        
        int mWidth;
        int mHeight;
	};
    
private:
    // neighbours offsets (for border spreading)
	int mDx4[4];
	int mDy4[4];
    
    // neighbours offsets (for sampling)
	vector<int> mDxs;
	vector<int> mDys;
    
	// distance Map to the unmasked part of the image
	Channel *mDistmap;
    
	// Output image
	Channel *mOutput;
	
    int mWidth;
	int mHeight;
    
	// mask color
	BL_FLOAT mMaskColor/*[3]*/;
    
	// isophote preservation factor
	int mPreservation;
    
    // Niko
    int mSampleRegionSize;
};

#endif /* defined(__BL_Ghost__IsophoteInpaint__) */
