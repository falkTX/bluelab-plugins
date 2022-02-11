#ifndef FAST_LOG2_H
#define FAST_LOG2_H

// See: https://tech.ebayinc.com/engineering/fast-approximate-logarithms-part-i-the-basics
//
// The code does not check that x > 0, much less check for infinities or NaNs.

extern float fastlog2(float x);

#endif
