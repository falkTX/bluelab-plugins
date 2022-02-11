#ifndef CFX_RBJ_FILTER_H
#define CFX_RBJ_FILTER_H

#include <Utils.h> // Hackish include...
#include <BLTypes.h>

#include <math.h>

// Niko
// See: https://forum.cockos.com/showthread.php?t=121016
// " If you put two filters back-to-back with a Q=0.707, you should get a 4th order filter that sums flat."
//
// Note: no phase inversions for 4th order
// See: https://en.wikipedia.org/wiki/Linkwitz%E2%80%93Riley_filter

// See also (for usage): https://www.kvraudio.com/forum/viewtopic.php?t=286084

#define FILTER_TYPE_LOWPASS         0
#define FILTER_TYPE_HIPASS          1
#define FILTER_TYPE_BANDPASS_CSG    2
#define FILTER_TYPE_BANDPASS_CZPG   3
#define FILTER_TYPE_NOTCH           4
#define FILTER_TYPE_ALLPASS         5
#define FILTER_TYPE_PEAKING         6
#define FILTER_TYPE_LOWSHELF        7
#define FILTER_TYPE_HISHELF         8

#ifndef SAMPLE_TYPE
#define SAMPLE_TYPE double
#endif

class CFxRbjFilter2X;
class CFxRbjFilter
{
public:
	
	CFxRbjFilter()
	{
		// reset filter coeffs
		b0a0=b1a0=b2a0=a1a0=a2a0=0.0;
		
		// reset in/out history
		ou1=ou2=in1=in2=0.0f;	
	};

    void reset()
    {
        // reset in/out history
        ou1=ou2=in1=in2=0.0f;
    }
    
	SAMPLE_TYPE filter(SAMPLE_TYPE in0)
	{
        FIX_FLT_DENORMAL(in0)

		// filter
		SAMPLE_TYPE yn = b0a0*in0 + b1a0*in1 + b2a0*in2 - a1a0*ou1 - a2a0*ou2;

        FIX_FLT_DENORMAL(yn)
        
		// push in/out buffers
		in2=in1;
		in1=in0;
		ou2=ou1;
		ou1=yn;
        
		// return output
		return yn;
	};

    void filter(SAMPLE_TYPE *ioSamples, int nSamples)
    {
        for (int i = 0; i  < nSamples; i++)
        {
            SAMPLE_TYPE in0 = ioSamples[i];
            
            FIX_FLT_DENORMAL(in0)
        
            // filter
            SAMPLE_TYPE yn = b0a0*in0 + b1a0*in1 + b2a0*in2 - a1a0*ou1 - a2a0*ou2;
        
            FIX_FLT_DENORMAL(yn)
        
            // push in/out buffers
            in2=in1;
            in1=in0;
            ou2=ou1;
            ou1=yn;
        
            ioSamples[i] = yn;
        }
    };
    
	void calc_filter_coeffs(int const type,SAMPLE_TYPE const frequency,SAMPLE_TYPE const sample_rate,SAMPLE_TYPE const q,SAMPLE_TYPE const db_gain,bool q_is_bandwidth)
	{
		// temp pi
		SAMPLE_TYPE const temp_pi=3.1415926535897932384626433832795;
		
		// temp coef vars
		SAMPLE_TYPE alpha,a0,a1,a2,b0,b1,b2;

		// peaking, lowshelf and hishelf
		if(type>=6)
		{
			SAMPLE_TYPE const A		=	std::pow((SAMPLE_TYPE)10.0, (SAMPLE_TYPE)((db_gain/40.0)));
			SAMPLE_TYPE const omega	=	2.0*temp_pi*frequency/sample_rate;
			SAMPLE_TYPE const tsin	=	std::sin(omega);
			SAMPLE_TYPE const tcos	=	std::cos(omega);
			
			if(q_is_bandwidth)
			alpha=tsin*std::sinh((SAMPLE_TYPE)(std::log((SAMPLE_TYPE)2.0)/2.0*q*omega/tsin));
			else
			alpha=tsin/(2.0*q);

			SAMPLE_TYPE const beta	=	std::sqrt(A)/q;
			
			// peaking
			if(type==6)
			{
				b0=SAMPLE_TYPE(1.0+alpha*A);
				b1=SAMPLE_TYPE(-2.0*tcos);
				b2=SAMPLE_TYPE(1.0-alpha*A);
				a0=SAMPLE_TYPE(1.0+alpha/A);
				a1=SAMPLE_TYPE(-2.0*tcos);
				a2=SAMPLE_TYPE(1.0-alpha/A);
			}
			
			// lowshelf
			if(type==7)
			{
				b0=SAMPLE_TYPE(A*((A+1.0)-(A-1.0)*tcos+beta*tsin));
				b1=SAMPLE_TYPE(2.0*A*((A-1.0)-(A+1.0)*tcos));
				b2=SAMPLE_TYPE(A*((A+1.0)-(A-1.0)*tcos-beta*tsin));
				a0=SAMPLE_TYPE((A+1.0)+(A-1.0)*tcos+beta*tsin);
				a1=SAMPLE_TYPE(-2.0*((A-1.0)+(A+1.0)*tcos));
				a2=SAMPLE_TYPE((A+1.0)+(A-1.0)*tcos-beta*tsin);
			}

			// hishelf
			if(type==8)
			{
				b0=SAMPLE_TYPE(A*((A+1.0)+(A-1.0)*tcos+beta*tsin));
				b1=SAMPLE_TYPE(-2.0*A*((A-1.0)+(A+1.0)*tcos));
				b2=SAMPLE_TYPE(A*((A+1.0)+(A-1.0)*tcos-beta*tsin));
				a0=SAMPLE_TYPE((A+1.0)-(A-1.0)*tcos+beta*tsin);
				a1=SAMPLE_TYPE(2.0*((A-1.0)-(A+1.0)*tcos));
				a2=SAMPLE_TYPE((A+1.0)-(A-1.0)*tcos-beta*tsin);
			}
		}
		else
		{
			// other filters
			SAMPLE_TYPE const omega	=	2.0*temp_pi*frequency/sample_rate;
			SAMPLE_TYPE const tsin	=	std::sin(omega);
			SAMPLE_TYPE const tcos	=	std::cos(omega);

			if(q_is_bandwidth)
			alpha=tsin*std::sinh((SAMPLE_TYPE)(std::log((SAMPLE_TYPE)2.0)/2.0*q*omega/tsin));
			else
			alpha=tsin/(2.0*q);

#if 1 // ORIG
			// lowpass
			if(type==0)
			{
				b0=(1.0-tcos)/2.0;
				b1=1.0-tcos;
				b2=(1.0-tcos)/2.0;
				a0=1.0+alpha;
				a1=-2.0*tcos;
				a2=1.0-alpha;
			}

			// hipass
			if(type==1)
			{
				b0=(1.0+tcos)/2.0;
				b1=-(1.0+tcos);
				b2=(1.0+tcos)/2.0;
				a0=1.0+ alpha;
				a1=-2.0*tcos;
				a2=1.0-alpha;
			}
#else
            // Patch
            // See: https://forum.cockos.com/showthread.php?t=121016
            // "The other thing you could do is modify the RBJ filter coefficient code to make a specific LR2 filter for the 2nd order biquad calculation."
            
            {
                const SAMPLE_TYPE omega = temp_pi * frequency;
                const SAMPLE_TYPE kappa = omega / tan(temp_pi * frequency / sample_rate);
                const SAMPLE_TYPE delta = (kappa * kappa) + (omega * omega) + (2.0 * kappa * omega);
                a0 = delta;
                a1 = -2.0 * kappa * kappa + 2.0 * omega * omega;
                a2 = -2.0 * kappa * omega + kappa * kappa + omega * omega;
			
			if (type == 0)
            {
				b0 = omega * omega;
				b1 = 2.0 * b0;
				b2 = b0;
            }
			else if (type == 1)
            {
				b0 = kappa * kappa;
				b1 = -2.0 * kappa * kappa;
				b2 = b0;
            }
        }
#endif
            
			// bandpass csg
			if(type==2)
			{
				b0=tsin/2.0;
				b1=0.0;
			    b2=-tsin/2;
				a0=1.0+alpha;
				a1=-2.0*tcos;
				a2=1.0-alpha;
			}

			// bandpass czpg
			if(type==3)
			{
				b0=alpha;
				b1=0.0;
				b2=-alpha;
				a0=1.0+alpha;
				a1=-2.0*tcos;
				a2=1.0-alpha;
			}

			// notch
			if(type==4)
			{
				b0=1.0;
				b1=-2.0*tcos;
				b2=1.0;
				a0=1.0+alpha;
				a1=-2.0*tcos;
				a2=1.0-alpha;
			}

			// allpass
			if(type==5)
			{
				b0=1.0-alpha;
				b1=-2.0*tcos;
				b2=1.0+alpha;
				a0=1.0+alpha;
				a1=-2.0*tcos;
				a2=1.0-alpha;
			}
		}
        
		// set filter coeffs
		b0a0=SAMPLE_TYPE(b0/a0);
		b1a0=SAMPLE_TYPE(b1/a0);
		b2a0=SAMPLE_TYPE(b2/a0);
		a1a0=SAMPLE_TYPE(a1/a0);
		a2a0=SAMPLE_TYPE(a2/a0);
	};

private:
    friend class CFxRbjFilter2X;
    
	// filter coeffs
	SAMPLE_TYPE b0a0,b1a0,b2a0,a1a0,a2a0;

	// in/out history
	SAMPLE_TYPE ou1,ou2,in1,in2;
};

// #bluelab
// Chain 2 filters intelligently
class CFxRbjFilter2X
{
public:
    CFxRbjFilter2X()
    {
        for (int k = 0; k < 2; k++)
        {
            b0a0[k] = 0.0f;
            b1a0[k] = 0.0f;
            b2a0[k] = 0.0f;
            a1a0[k] = 0.0f;
            a2a0[k] = 0.0f;
            
            ou1[k] = 0.0f;
            ou2[k] = 0.0f;
            in1[k] = 0.0f;
            in2[k] = 0.0f;
        }
    }
    
    void reset()
    {
        for (int k = 0; k < 2; k++)
        {
            ou1[k] = 0.0f;
            ou2[k] = 0.0f;
            in1[k] = 0.0f;
            in2[k] = 0.0f;
        }
    }
    
    SAMPLE_TYPE filter(SAMPLE_TYPE in0)
    {
        // 1
        //
        FIX_FLT_DENORMAL(in0)
        
        // Filter
        SAMPLE_TYPE yn = b0a0[0]*in0 + b1a0[0]*in1[0] +
                         b2a0[0]*in2[0] - a1a0[0]*ou1[0] -
                         a2a0[0]*ou2[0];
        
        FIX_FLT_DENORMAL(yn)
        
        // push in/out buffers
        in2[0] = in1[0];
        in1[0] = in0;
        ou2[0] = ou1[0];
        ou1[0] = yn;
        
        // Result
        in0 = yn;
        
        // 2
        //
        FIX_FLT_DENORMAL(in0)
        
        // Filter
        yn = b0a0[1]*in0 + b1a0[1]*in1[1] +
        b2a0[1]*in2[1] - a1a0[1]*ou1[1] -
        a2a0[1]*ou2[1];
        
        FIX_FLT_DENORMAL(yn)
        
        // push in/out buffers
        in2[1] = in1[1];
        in1[1] = in0;
        ou2[1] = ou1[1];
        ou1[1] = yn;
        
        return yn;
    };
    
    void filter(SAMPLE_TYPE *ioSamples, int nSamples)
    {
        for (int i = 0; i < nSamples; i++)
        {
            SAMPLE_TYPE in0 = ioSamples[i];
        
            // 1
            //
            FIX_FLT_DENORMAL(in0)
        
            // Filter
            SAMPLE_TYPE yn = b0a0[0]*in0 + b1a0[0]*in1[0] + b2a0[0]*in2[0]
                                         - a1a0[0]*ou1[0] - a2a0[0]*ou2[0];
        
            FIX_FLT_DENORMAL(yn)
        
            // push in/out buffers
            in2[0] = in1[0];
            in1[0] = in0;
            ou2[0] = ou1[0];
            ou1[0] = yn;
        
            // Result
            in0 = yn;
        
            // 2
            //
            FIX_FLT_DENORMAL(in0)
        
            // Filter
            yn = b0a0[1]*in0 + b1a0[1]*in1[1] + b2a0[1]*in2[1]
                             - a1a0[1]*ou1[1] - a2a0[1]*ou2[1];
        
            FIX_FLT_DENORMAL(yn)
        
            // push in/out buffers
            in2[1] = in1[1];
            in1[1] = in0;
            ou2[1] = ou1[1];
            ou1[1] = yn;
        
            ioSamples[i] = yn;
        }
    };

    void
    calc_filter_coeffs(int const type,SAMPLE_TYPE const frequency,
                       SAMPLE_TYPE const sample_rate,
                       SAMPLE_TYPE const q,
                       SAMPLE_TYPE const db_gain,
                       bool q_is_bandwidth)
    {
        mFilter.calc_filter_coeffs(type, frequency,
                                   sample_rate,
                                   q,
                                   db_gain,
                                   q_is_bandwidth);
     
        set_coeffs();
    }
    
protected:
    void
    set_coeffs()
    {
        for (int k = 0; k < 2; k++)
        {
            b0a0[k] = mFilter.b0a0;
            b1a0[k] = mFilter.b1a0;
            b2a0[k] = mFilter.b2a0;
            a1a0[k] = mFilter.a1a0;
            a2a0[k] = mFilter.a2a0;
            
            /*ou1[k] = mFilter.ou1;
            ou2[k] = mFilter.ou2;
            in1[k] = mFilter.in1;
            in2[k] = mFilter.in2;*/
        }
    }
    
    //
    CFxRbjFilter mFilter;
    
    // filter coeffs
    SAMPLE_TYPE b0a0[2];
    SAMPLE_TYPE b1a0[2];
    SAMPLE_TYPE b2a0[2];
    SAMPLE_TYPE a1a0[2];
    SAMPLE_TYPE a2a0[2];
    
    // in/out history
    SAMPLE_TYPE ou1[2];
    SAMPLE_TYPE ou2[2];
    SAMPLE_TYPE in1[2];
    SAMPLE_TYPE in2[2];
};

#endif
