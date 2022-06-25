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
//  TestSimd.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 4/17/20.
//
//

#include "IPlug_include_in_plug_hdr.h"

#include <BLUtils.h>
#include <BLDebug.h>

#include "TestSimd.h"

#define MAKE_TEST(__TEST_NUM__)                         \
{ WDL_TypedBuf<BL_FLOAT> results[2];                      \
for (int i = 0; i < 2; i++)                             \
{ BLUtils::SetUseSimdFlag((i == 1));                      \
    Test##__TEST_NUM__(&results[i]); }                  \
if (!Compare(results))                                  \
{ fprintf(stderr, "Test-%d failed!\n", __TEST_NUM__);   \
    return; } }

TestSimd::TestSimd(bool activateSimd)
{
    //BLUtils::SetUseSimdFlag(activateSimd);
    
    //BLUtils::SetUseSimdFlag(false);
    BLUtils::SetUseSimdFlag(true);
}

TestSimd::~TestSimd() {}

void
TestSimd::Test()
{
    //TestPerfs();
    //return;
    
    // Unit tests
    /*WDL_TypedBuf<BL_FLOAT> results[2];
    for (int i = 0; i < 2; i++)
    {
        BLUtils::SetUseSimdFlag((i == 1));
        Test0(&results[i]);
    }
    if (!Compare(results))
    {
        fprintf(stderr, "Test0 failed!\n");
        return;
    }*/
    
    //MAKE_TEST(0);
    
    //MultValues(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf, BL_FLOAT value)
    //BLUtils::MultValues(WDL_TypedBuf<BL_FLOAT> *buf, const WDL_TypedBuf<BL_FLOAT> &values)
    //BLUtils::ComputeAbs(WDL_TypedBuf<BL_FLOAT> *values)
    //BLUtils::ComputeSum(const WDL_TypedBuf<BL_FLOAT> &buf)
    //ComputeRMSAvg(const BL_FLOAT *values, int nFrames)
    //ComputeSquareSum(const BL_FLOAT *values, int nFrames)
    //ComputeRMSAvg2(const BL_FLOAT *values, int nFrames)
    //ComputeAvg(const BL_FLOAT *buf, int nFrames)
    //BL_FLOAT ComputeSum(const BL_FLOAT *buf, int nFrames);
    //ComputeAvgSquare(const BL_FLOAT *buf, int nFrames)
    //BLUtils::ComputeSquare(WDL_TypedBuf<BL_FLOAT> *buf)
    //ComputeAbsAvg(const BL_FLOAT *buf, int nFrames)
    //ComputeAbsSum(const WDL_TypedBuf<BL_FLOAT> &buf)
    //ComputeAbsSum(const BL_FLOAT *buf, int nFrames)
    //ComputeMax(const BL_FLOAT *output, int nFrames)
    //BLUtils::ComputeMax(const vector<WDL_TypedBuf<BL_FLOAT> > &values)
    //BLUtils::ComputeMaxAbs(const BL_FLOAT *buf, int nFrames)
}

bool
TestSimd::Compare(const WDL_TypedBuf<BL_FLOAT> results[2])
{
#define EPS 1e-15
    
    if (results[0].GetSize() != results[1].GetSize())
        return false;
    
    for (int i = 0; i < results[0].GetSize(); i++)
    {
      if (std::fabs(results[0].Get()[i] - results[1].Get()[i]) > EPS)
        {
            BLDebug::DumpData("res0.txt", results[0]);
            BLDebug::DumpData("res1.txt", results[1]);
            
            return false;
        }
        
    }
    
    return true;
}

void
TestSimd::TestPerfs()
{
#define NUM_ITERS 2048
    
#define BUFFER_SIZE 1024
    
    WDL_TypedBuf<BL_FLOAT> data;
    data.Resize(BUFFER_SIZE);
    for (int i = 0; i < data.GetSize(); i++)
    {
        data.Get()[i] = i;
    }
    
    WDL_TypedBuf<BL_FLOAT> result;
    result.Resize(BUFFER_SIZE);
    for (int i = 0; i < NUM_ITERS; i++)
    {
        //BLUtils::FillAllZero(&result);
        result = data;
        
        BLUtils::MultValues(&result, (BL_FLOAT)2.0);
    }
}

void
TestSimd::Test0(WDL_TypedBuf<BL_FLOAT> *result)
{
#define BUFFER_SIZE 1024
    
    WDL_TypedBuf<BL_FLOAT> data;
    data.Resize(BUFFER_SIZE);
    for (int i = 0; i < data.GetSize(); i++)
    {
        data.Get()[i] = i;
    }
    
    *result = data;
        
    BLUtils::MultValues(result, (BL_FLOAT)2.0);
}
