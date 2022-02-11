// Macro for killing denormalled numbers
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// Based on IS_DENORMAL macro by Jon Watte
// This code is public domain

// Niko: see https://www.musicdsp.org/en/latest/Other/88-denormal-double-variables-macro.html
// in case of need double
// NOTE: undenormalise() is never used in freeverb

#ifndef _denormals_
#define _denormals_

#define undenormalise(sample) if(((*(unsigned int*)&sample)&0x7f800000)==0) sample=0.0f

#endif//_denormals_

//ends
