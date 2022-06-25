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
//  EarlyReflections2.h
//  BL-ReverbDepth
//
//  Created by applematuer on 9/1/20.
//
//

#ifndef __BL_ReverbDepth__EarlyReflections2__
#define __BL_ReverbDepth__EarlyReflections2__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

// See: https://reuk.github.io/wayverb/image_source.html
//
// and also(for cusiosity): https://arxiv.org/pdf/1803.00430.pdf
//
class DelayObj4;
class EarlyReflections2
{
public:
    EarlyReflections2(BL_FLOAT sampleRate);
    
    EarlyReflections2(const EarlyReflections2 &other);
    
    virtual ~EarlyReflections2();
    
    void Reset(BL_FLOAT sampleRate);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &samples,
                 WDL_TypedBuf<BL_FLOAT> outRevSamples[2]);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &samples,
                 WDL_TypedBuf<BL_FLOAT> *outRevSamplesL,
                 WDL_TypedBuf<BL_FLOAT> *outRevSamplesR);
    
    //
    void SetOrder(int order); //
    
    void SetRoomSize(BL_FLOAT roomSize);
    void SetIntermicDist(BL_FLOAT dist);
    void SetNormDepth(BL_FLOAT depth);
    
    // Material reflection coefficient
    void SetReflectCoeff(BL_FLOAT coeff); //
    
protected:
    //
    class Ray
    {
    public:
        Ray();
        
        Ray(const Ray &other);
        
        virtual ~Ray();
        
        //
        BL_FLOAT mPoint[2];
        
        DelayObj4 *mDelay;
        BL_FLOAT mAttenCoeff;
    };
    
    //
    struct Point
    {
        BL_FLOAT mValues[2];
    };
    
    //
    void Init();
    
    void GenerateRays(vector<Ray> *rays,
                      BL_FLOAT roomCorners[4][2],
                      BL_FLOAT S[2], BL_FLOAT R[2]);
    
    void GenerateRays(vector<Ray> *newRays,
                      BL_FLOAT roomCorners[4][2],
                      BL_FLOAT S[2], BL_FLOAT R[2],
                      const vector<Ray> &prevRays,
                      int order);
    
    //
    BL_FLOAT mSampleRate;
    
    vector<Ray> mRays[2];
    
    int mOrder;
    
    BL_FLOAT mRoomSize;
    BL_FLOAT mIntermicDist;
    BL_FLOAT mNormDepth;
    
    BL_FLOAT mReflectCoeff;
};

#endif /* defined(__BL_ReverbDepth__EarlyReflections2__) */
