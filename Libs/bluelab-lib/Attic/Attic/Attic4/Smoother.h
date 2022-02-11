//
//  Smooth.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#ifndef __Transient__Smooth__
#define __Transient__Smooth__

#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"

class Smoother
{
public:
    static void Smooth(const WDL_TypedBuf<BL_FLOAT> *iBuf, WDL_TypedBuf<BL_FLOAT> *oBuf, int smoothSize);
    
    static void Smooth2(const WDL_TypedBuf<BL_FLOAT> *iBuf, WDL_TypedBuf<BL_FLOAT> *oBuf, BL_FLOAT smoothCoeff);
    
    static void Smooth3(const WDL_TypedBuf<BL_FLOAT> *iBuf, WDL_TypedBuf<BL_FLOAT> *oBuf, int smoothSize);
};

#endif /* defined(__Transient__Smooth__) */
