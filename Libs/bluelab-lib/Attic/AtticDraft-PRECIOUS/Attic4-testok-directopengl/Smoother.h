//
//  Smooth.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#ifndef __Transient__Smooth__
#define __Transient__Smooth__

#include "../../WDL/IPlug/Containers.h"

class Smoother
{
public:
    static void Smooth(const WDL_TypedBuf<double> *iBuf, WDL_TypedBuf<double> *oBuf, int smoothSize);
    
    static void Smooth2(const WDL_TypedBuf<double> *iBuf, WDL_TypedBuf<double> *oBuf, double smoothCoeff);
    
    static void Smooth3(const WDL_TypedBuf<double> *iBuf, WDL_TypedBuf<double> *oBuf, int smoothSize);
};

#endif /* defined(__Transient__Smooth__) */
