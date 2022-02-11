//
//  ScopedNoDenormals.h
//  UST
//
//  Created by applematuer on 8/17/20.
//
//

#ifndef UST_ScopedNoDenormals_h
#define UST_ScopedNoDenormals_h

#include "xmmintrin.h"

// See: https://forum.juce.com/t/state-of-the-art-denormal-prevention/16802
class ScopedNoDenormals
{
public:
    ScopedNoDenormals()
    {
        
        //There is also C99 way of doing this, but its not widely supported: fesetenv(...)
        
        oldMXCSR = _mm_getcsr(); /*read the old MXCSR setting */ \
        int newMXCSR = oldMXCSR | 0x8040; /* set DAZ and FZ bits */ \
        _mm_setcsr( newMXCSR); /*write the new MXCSR setting to the MXCSR */
    };
    
    ~ScopedNoDenormals()
    {
        _mm_setcsr( oldMXCSR );
    };
    
private:
    int oldMXCSR;
};

#endif
