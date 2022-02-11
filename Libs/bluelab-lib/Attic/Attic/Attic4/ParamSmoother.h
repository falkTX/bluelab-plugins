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
