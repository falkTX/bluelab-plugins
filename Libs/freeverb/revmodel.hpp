// Reverb model declaration
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain

// adapted for use in VCV Rack by Martin Lueders

#ifndef _revmodel_
#define _revmodel_

#include <math.h>

#include "comb.hpp"
#include "allpass.hpp"
#include "tuning.h"

class revmodel
{
public:
	revmodel();

	void	init(const FV_FLOAT sampleRate);

	void	mute();

	void	process(const FV_FLOAT input, FV_FLOAT &outputL, FV_FLOAT &outputR);

	void	setroomsize(FV_FLOAT value);
	FV_FLOAT	getroomsize();
	void	setdamp(FV_FLOAT value);
	FV_FLOAT	getdamp();
	void	setwet(FV_FLOAT value);
	FV_FLOAT	getwet();
	void	setdry(FV_FLOAT value);
	FV_FLOAT	getdry();
	void	setwidth(FV_FLOAT value);
	FV_FLOAT	getwidth();
	void	setmode(FV_FLOAT value);
	FV_FLOAT	getmode();
private:
	void	update();
private:
	FV_FLOAT	gain;
	FV_FLOAT	roomsize,roomsize1;
	FV_FLOAT	damp,damp1, damp2;
	FV_FLOAT   feedback_allpass;
	FV_FLOAT	wet,wet1,wet2;
	FV_FLOAT	dry;
	FV_FLOAT	width;
	FV_FLOAT	mode;

	FV_FLOAT 	conversion;
	FV_FLOAT math_e = 2.71828;

	// The following are all declared inline 
	// to remove the need for dynamic allocation
	// with its subsequent error-checking messiness

	// Comb filters
	comb	combL[numcombs];
	comb	combR[numcombs];

	// Allpass filters
	allpass	allpassL[numallpasses];
	allpass	allpassR[numallpasses];

	// Buffers for the combs
	FV_FLOAT	*bufcombL1;
	FV_FLOAT	*bufcombR1;
	FV_FLOAT	*bufcombL2;
	FV_FLOAT	*bufcombR2;
	FV_FLOAT	*bufcombL3;
	FV_FLOAT	*bufcombR3;
	FV_FLOAT	*bufcombL4;
	FV_FLOAT	*bufcombR4;
	FV_FLOAT	*bufcombL5;
	FV_FLOAT	*bufcombR5;
	FV_FLOAT	*bufcombL6;
	FV_FLOAT	*bufcombR6;
	FV_FLOAT	*bufcombL7;
	FV_FLOAT	*bufcombR7;
	FV_FLOAT	*bufcombL8;
	FV_FLOAT	*bufcombR8;

	// Buffers for the allpasses
	FV_FLOAT	*bufallpassL1;
	FV_FLOAT	*bufallpassR1;
	FV_FLOAT	*bufallpassL2;
	FV_FLOAT	*bufallpassR2;
	FV_FLOAT	*bufallpassL3;
	FV_FLOAT	*bufallpassR3;
	FV_FLOAT	*bufallpassL4;
	FV_FLOAT	*bufallpassR4;
};

#endif//_revmodel_

//ends
