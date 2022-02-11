// Comb filter class declaration
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain

// adapted for use in VCV Rack by Martin Lueders

#ifndef _comb_
#define _comb_

#include <BLTypes.h>

#include "denormals.h"

class comb
{
public:
	comb() 
	{
		buffer = 0;
		filterstore = 0;
		bufidx = 0;
	};

	~comb()
	{
		if (buffer) delete buffer;
	};

	void    makebuffer(FV_FLOAT *buf, int size) 
	{
		if (buffer) {delete buffer;}
		buffer = new FV_FLOAT[size];
		bufsize = size;
		bufidx = 0;
        
        // Niko
        mute();
	}

	void	deletebuffer()
	{
		if(buffer) delete buffer;
		bufsize = 0;
	};

	void	setbuffer(FV_FLOAT *buf, int size)
	{
		buffer = buf;
		bufsize = size;
	};

	inline  FV_FLOAT	process(FV_FLOAT inp, FV_FLOAT damp1, FV_FLOAT damp2, FV_FLOAT feedback);
	void	mute()
	{
		for( int i=0; i<bufsize; i++) buffer[i]=0.0;
	};

private:
	FV_FLOAT	filterstore;
	FV_FLOAT	*buffer;
	int	bufsize;
	int	bufidx;
};


// Big to inline - but crucial for speed

inline FV_FLOAT comb::process(FV_FLOAT input, FV_FLOAT damp1, FV_FLOAT damp2, FV_FLOAT feedback)
{
    // Niko
    if (bufsize == 0)
        return 0.0;
    
	FV_FLOAT output;

	output = buffer[bufidx]; // y[n-K]
	//undenormalise(output);

	filterstore *= damp1;
	filterstore += (output*damp2);
    
//  filterstore = damp1*filterstore + damp2*output;
//  filterstore = damp1*filterstore + (1.0-damp1) * output;
//	filterstore = output + damp1*(filterstore - output);



//	undenormalise(filterstore);

	buffer[bufidx] = input + (filterstore*feedback);
    
	if(++bufidx>=bufsize) bufidx = 0;

	return output;
}

#endif //_comb_

//ends
