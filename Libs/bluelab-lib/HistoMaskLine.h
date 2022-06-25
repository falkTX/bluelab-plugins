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
//  HistoMaskLine.h
//  BL-Panogram
//
//  Created by applematuer on 10/22/19.
//
//

#ifndef __BL_Panogram__HistoMaskLine__
#define __BL_Panogram__HistoMaskLine__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class HistoMaskLine
{
public:
    HistoMaskLine(int bufferSize);
    
    virtual ~HistoMaskLine();

    void AddValue(int index, int value);
    
    void Apply(WDL_TypedBuf<BL_FLOAT> *values,
               int startIndex, int endIndex);
    
protected:
    vector<vector<int> > mBuffer;
};

#endif /* defined(__BL_Panogram__HistoMaskLine__) */
