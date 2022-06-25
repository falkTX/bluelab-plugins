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
//  ParamSmoother.h
//  StereoPan
//
//  Created by Apple m'a Tuer on 09/04/17.
//
//

#ifndef StereoPan_ParamSmoother_h
#define StereoPan_ParamSmoother_h

#include <BLTypes.h>

#define DEFAULT_SMOOTH_COEFF 0.9
#define DEFAULT_PRECISION    10

class ParamSmoother
{
public:
    ParamSmoother(BL_FLOAT value, BL_FLOAT smoothCoeff = DEFAULT_SMOOTH_COEFF, int precision = DEFAULT_PRECISION);
    
    ParamSmoother();
    
    virtual ~ParamSmoother();
    
    void SetSmoothCoeff(BL_FLOAT smoothCoeff);
    
    BL_FLOAT GetCurrentValue();
    void SetNewValue(BL_FLOAT value);
    
    void Update();
    
    void Reset();
    
    void Reset(BL_FLOAT newValue);
    
    bool IsStable();
    
protected:
    bool mFirstValueSet;
    
    BL_FLOAT mSmoothCoeff;
    BL_FLOAT mCurrentValue;
    BL_FLOAT mNewValue;
    
    bool mIsStable;
    int mPrecision;
    
    // Before any update, we force the setting of new value without smoothing
    // Useful at the initialization of a plugin, when we get the previous save parameter values
    bool mFirstUpdate;
};

#endif
