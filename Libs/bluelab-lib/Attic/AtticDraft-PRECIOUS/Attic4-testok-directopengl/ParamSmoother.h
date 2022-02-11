//
//  ParamSmoother.h
//  StereoPan
//
//  Created by Apple m'a Tuer on 09/04/17.
//
//

#ifndef StereoPan_ParamSmoother_h
#define StereoPan_ParamSmoother_h

#define DEFAULT_SMOOTH_COEFF 0.9
#define DEFAULT_PRECISION    10

class ParamSmoother
{
public:
    ParamSmoother(double value, double smoothCoeff = DEFAULT_SMOOTH_COEFF, int precision = DEFAULT_PRECISION);
    
    ParamSmoother();
    
    virtual ~ParamSmoother();
    
    void SetSmoothCoeff(double smoothCoeff);
    
    double GetCurrentValue();
    void SetNewValue(double value);
    
    void Update();
    void Reset(double newValue);
    
    bool IsStable();
    
protected:
    bool mFirstValueSet;
    
    double mSmoothCoeff;
    double mCurrentValue;
    double mNewValue;
    
    bool mIsStable;
    int mPrecision;
    
    // Before any update, we force the setting of new value without smoothing
    // Useful at the initialization of a plugin, when we get the previous save parameter values
    bool mFirstUpdate;
};

#endif
