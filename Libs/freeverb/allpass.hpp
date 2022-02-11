// Allpass filter declaration
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain

// adapted for use in VCV Rack by Martin Lueders

#ifndef _allpass_
#define _allpass_
#include "denormals.h"

class allpass
{
public:
	allpass()
	{
		bufidx = 0;
		buffer = 0;
	};

	~allpass()
	{
		if (buffer) delete buffer;
	};

        void    makebuffer(FV_FLOAT *buf, int size)
        {
		if (buffer) delete buffer;
                buffer = new FV_FLOAT[size];
                bufsize = size;
		bufidx = 0;
        }

        void    deletebuffer()
        {
                if(buffer) delete buffer;
                bufsize = 0;
        };

	void	setbuffer(FV_FLOAT *buf, int size)
	{
		buffer = buf;
		bufsize = size;
	};

	inline  FV_FLOAT	process(FV_FLOAT inp, FV_FLOAT feedback);
	void	mute()
	{
		for (int i=0; i<bufsize; i++) buffer[i] = 0.0;
	};

// private:
	FV_FLOAT	*buffer;
	int	bufsize;
	int	bufidx;
};


// Big to inline - but crucial for speed

inline FV_FLOAT allpass::process(FV_FLOAT input, FV_FLOAT feedback)
{
	FV_FLOAT output;
	FV_FLOAT bufout;
	
	bufout = buffer[bufidx];
//	undenormalise(bufout);
	
	output = -input + bufout;
	buffer[bufidx] = input + (bufout*feedback);

	if(++bufidx>=bufsize) bufidx = 0;

	return output;
}

#endif//_allpass

//ends
