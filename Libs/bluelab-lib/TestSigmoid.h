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
 
#ifndef TEST_SIGMOID_H
#define TEST_SIGMOID_H

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

class TestSigmoid
{
 public:
    // Sigmoid
    static void RunTest0()
    {
        int numValues = 1024;
        //BL_FLOAT a = 0.5;
        //BL_FLOAT a = 0.2;
        BL_FLOAT a = 0.05;
        
        WDL_TypedBuf<BL_FLOAT> tVec;
        tVec.Resize(numValues);

        for (int i = 0; i < tVec.GetSize(); i++)
        {
            BL_FLOAT t = ((BL_FLOAT)i)/(numValues + 1);
            tVec.Get()[i] = t;
        }

        BLDebug::DumpData("data0.txt", tVec);

        WDL_TypedBuf<BL_FLOAT> tVecSigmo = tVec;
        for (int i = 0; i < tVecSigmo.GetSize(); i++)
        {
            BL_FLOAT t = tVec.Get()[i];

            t = BLUtilsMath::ApplySigmoid(t, a);

            tVecSigmo.Get()[i] = t;
        }

        BLDebug::DumpData("data1.txt", tVecSigmo);

        WDL_TypedBuf<BL_FLOAT> tVecSigmoInv = tVecSigmo;
        for (int i = 0; i < tVecSigmoInv.GetSize(); i++)
        {
            BL_FLOAT t = tVecSigmo.Get()[i];

            t = BLUtilsMath::ApplySigmoid(t, 1.0 - a);

            tVecSigmoInv.Get()[i] = t;
        }

        BLDebug::DumpData("data2.txt", tVecSigmoInv);
    }

    // Gamma
    static void RunTest1()
    {
        int numValues = 1024;
        //BL_FLOAT a = 0.5;
        //BL_FLOAT a = 0.2;
        BL_FLOAT a = 0.05;
        
        WDL_TypedBuf<BL_FLOAT> tVec;
        tVec.Resize(numValues);

        for (int i = 0; i < tVec.GetSize(); i++)
        {
            BL_FLOAT t = ((BL_FLOAT)i)/(numValues + 1);
            tVec.Get()[i] = t;
        }

        BLDebug::DumpData("data0.txt", tVec);

        WDL_TypedBuf<BL_FLOAT> tVecGamma = tVec;
        for (int i = 0; i < tVecGamma.GetSize(); i++)
        {
            BL_FLOAT t = tVec.Get()[i];

            t = BLUtilsMath::ApplyGamma(t, a);

            tVecGamma.Get()[i] = t;
        }

        BLDebug::DumpData("data1.txt", tVecGamma);

        WDL_TypedBuf<BL_FLOAT> tVecGammaInv = tVecGamma;
        for (int i = 0; i < tVecGammaInv.GetSize(); i++)
        {
            BL_FLOAT t = tVecGamma.Get()[i];

            t = BLUtilsMath::ApplyGamma(t, 1.0 - a);

            tVecGammaInv.Get()[i] = t;
        }

        BLDebug::DumpData("data2.txt", tVecGammaInv);
    }
};

#endif
