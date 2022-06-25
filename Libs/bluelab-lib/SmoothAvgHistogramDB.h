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
//  AvgHistogram.h
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#ifndef EQHack_SmoothAvgHistogramDB_h
#define EQHack_SmoothAvgHistogramDB_h

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"
#include "IControl.h"
//#include "../../WDL/IPlug/Containers.h"

// Normalize Y to dB internally
class SmoothAvgHistogramDB
{
public:
    SmoothAvgHistogramDB(int size, BL_FLOAT smoothCoeff,
                         BL_FLOAT defaultValue, BL_FLOAT mindB, BL_FLOAT maxdB);
    
    virtual ~SmoothAvgHistogramDB();
    
    void AddValue(int index, BL_FLOAT val);
    
    void AddValues(const WDL_TypedBuf<BL_FLOAT> &values);
    
    int GetNumValues();

    // Get values scaled back to amp
    void GetValues(WDL_TypedBuf<BL_FLOAT> *values);

    // OPTIM: Get internal values, which are in DB
    void GetValuesDB(WDL_TypedBuf<BL_FLOAT> *values);
    
    // Force the internal values to be the new values,
    // (withtout smoothing)
    void SetValues(const WDL_TypedBuf<BL_FLOAT> *values,
                   bool convertToDB = false);
    
    void Reset(BL_FLOAT smoothCoeff = -1.0);
    
protected:
    WDL_TypedBuf<BL_FLOAT> mData;

    // If smooth is 0, then we get instantaneous value.
    // If smooth is 1, then we get total avg
    BL_FLOAT mSmoothCoeff;
    
    BL_FLOAT mDefaultValue;
    
    BL_FLOAT mMindB;
    BL_FLOAT mMaxdB;

private:
    // Tmp buffers
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
};

#endif
