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

#ifndef EQHack_MinMaxAvgHistogramDB_h
#define EQHack_MinMaxAvgHistogramDB_h

#include "IControl.h"

class MinMaxAvgHistogramDB
{
public:
    MinMaxAvgHistogramDB(int size, BL_FLOAT smoothCoeff, BL_FLOAT defaultValue,
                         BL_FLOAT mindB, BL_FLOAT maxdB);
    
    virtual ~MinMaxAvgHistogramDB();
    
    void AddValue(int index, BL_FLOAT val);
    
    void AddValues(WDL_TypedBuf<BL_FLOAT> *values);
    
    void GetMinValues(WDL_TypedBuf<BL_FLOAT> *values);
    
    void GetMaxValues(WDL_TypedBuf<BL_FLOAT> *values);
    
    void GetAvgValues(WDL_TypedBuf<BL_FLOAT> *values);
    
    void Reset();
    
protected:
    WDL_TypedBuf<BL_FLOAT> mMinData;
    WDL_TypedBuf<BL_FLOAT> mMaxData;
    
    // If smooth is 0, then we get instantaneous value.
    // If smooth is 1, then we get total avg
    BL_FLOAT mSmoothCoeff;
    
    BL_FLOAT mDefaultValue;
    
    BL_FLOAT mMindB;
    BL_FLOAT mMaxdB;
};

#endif
