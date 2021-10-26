#ifndef KNOB_SMOOTHER_H
#define KNOB_SMOOTHER_H

#include <BLTypes.h>
#include <ParamSmoother2.h>

#include "IPlug_include_in_plug_hdr.h"
using namespace iplug;


#define DEFAULT_SMOOTHING_TIME_MS 140.0

// More convenient smoothing of params.
// Smooth the normalized param value
// (seems more homogeneous whatever the parameter non normalized value)
// Seems to manage well when param has a pow shape
// (ParamSmoother2 only makes step scales e.g with BL-Sine)
class KnobSmoother
{
public:
    KnobSmoother(Plugin *plug, int paramIdx,
                 BL_FLOAT sampleRate,
                 BL_FLOAT smoothingTimeMs = DEFAULT_SMOOTHING_TIME_MS)
    {
        mPlug = plug;
        mParamIdx = paramIdx;
        
        BL_FLOAT value = plug->GetParam(mParamIdx)->GetNormalized();
        mParamSmoother = new ParamSmoother2(sampleRate, value, smoothingTimeMs);
                                            
    }
    
    virtual ~KnobSmoother()
    {
        delete mParamSmoother;
    }
    
    inline void Reset(BL_FLOAT sampleRate, BL_FLOAT smoothingTimeMs = -1.0)
    {
        mParamSmoother->Reset(sampleRate, smoothingTimeMs);
    }

    inline void ResetToTargetValue()
    {
        BL_FLOAT value = mPlug->GetParam(mParamIdx)->GetNormalized();
        mParamSmoother->ResetToTargetValue(value);
    }
    
    inline BL_FLOAT Process()
    {
        BL_FLOAT value = mPlug->GetParam(mParamIdx)->GetNormalized();
        mParamSmoother->SetTargetValue(value);

        BL_FLOAT value1 = mParamSmoother->Process();
        
        BL_FLOAT result = mPlug->GetParam(mParamIdx)->FromNormalized(value1);
        
        return result;
    }

    inline BL_FLOAT PickCurrentValue()
    {
        BL_FLOAT value1 = mParamSmoother->PickCurrentValue();
        
        BL_FLOAT result = mPlug->GetParam(mParamIdx)->FromNormalized(value1);
        
        return result;
    }
    
protected:
    Plugin *mPlug;

    int mParamIdx;
    
    ParamSmoother2 *mParamSmoother;
};

#endif
