//
//  UpTime.h
//  BL-PitchShift
//
//  Created by Pan on 30/07/18.
//
//

#ifndef __BL_PitchShift__UpTime__
#define __BL_PitchShift__UpTime__

// Niko
class UpTime
{
public:
    static unsigned long long GetUpTime();

    // Use double, not float
    // double has a precision of 16 decimals (float has only 7)
    // With double, we can store 1M seconds, with a precision on 1 ns
    static double GetUpTimeF();
};

#endif /* defined(__BL_PitchShift__UpTime__) */
