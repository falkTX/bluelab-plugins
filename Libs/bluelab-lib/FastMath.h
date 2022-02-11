#ifndef FAST_MATH_H
#define FAST_MATH_H

class FastMath
{
 public:
    // Create and object to enable or disable fast math in the current scope
    FastMath(bool flag);
    virtual ~FastMath();
    
    static void SetFast(bool flag);

    static float log(float x);
    static double log(double x);

    static float log10(float x);
    static double log10(double x);
    
    static float exp(float x);
    static double exp(double x);

    static float pow(float x, float p);
    static double pow(double x, double p);
};

#endif
