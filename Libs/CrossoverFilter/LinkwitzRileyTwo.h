// See: https://forum.cockos.com/showthread.php?t=121016

//
//  FILE: LinkwitzRileyTwo.h
//  NOTES: Modified second order Biquad X-Over 
//  -6dB rollof at cutoff-point
//  Built after example code used in "Design Audio Effect Plug-Ins in C++" by Will Pirkle, Chapter 13
//
//  Created by Christian Schragen on 07.10.13.
//
//  License: open source


#include <math.h>

#define FLT_MIN_PLUS          1.175494351e-38        
#define FLT_MIN_MINUS        -1.175494351e-38



class CLinkwitzTwo
{
public:
    
    CLinkwitzTwo(double sampleRate=44100.0,double crossoverFreq=440.0) : mSampleRate(sampleRate), mFreq(crossoverFreq)
    {
        calculateFilterbankCoeffs();
    }
    
    ~CLinkwitzTwo(void);
    
    
    void SetCrossoverFreq(const double newCrossoverFreq) {
        mFreq = newCrossoverFreq;
        calculateFilterbankCoeffs();
    }
    
    void SetSampleRate(const double newSampleRate) {
        mSampleRate = newSampleRate;
        calculateFilterbankCoeffs();
    }

    
    double m_f_a0_LP;
	double m_f_a1_LP;
	double m_f_a2_LP;
	double m_f_b1_LP;
	double m_f_b2_LP;
    
        double m_f_a0_HP;
	double m_f_a1_HP;
	double m_f_a2_HP;
	double m_f_b1_HP;
	double m_f_b2_HP;
    
	// flush Delays
	void flushDelays()
	{
		m_f_Xz_1 = 0;
		m_f_Xz_2 = 0;
		m_f_Yz_1_LP = 0;
		m_f_Yz_2_LP = 0;
        
                m_f_Yz_1_HP = 0;
		m_f_Yz_2_HP = 0;
	}
    

    void Process(double &sample,double &lowOut,double &highOut)
	{
        static double smp;
        smp = sample;
		// difference equation: y(n) = a0x(n) + a1x(n-1) + a2x(n-2) - b1y(n-1) - b2y(n-2)
		
        lowOut = m_f_a0_LP*sample + m_f_a1_LP*m_f_Xz_1 + m_f_a2_LP*m_f_Xz_2 - m_f_b1_LP*m_f_Yz_1_LP - m_f_b2_LP*m_f_Yz_2_LP;

        highOut = m_f_a0_HP*sample + m_f_a1_HP*m_f_Xz_1 + m_f_a2_HP*m_f_Xz_2 - m_f_b1_HP*m_f_Yz_1_HP - m_f_b2_HP*m_f_Yz_2_HP;

        //-- NOTE: YOU HAVE TO INVERT ONE OF THEM, HIGH OR LOW, IN YOUR PROCESS LOOP!
        
		// underflow check
		if(lowOut > 0.0 && lowOut < FLT_MIN_PLUS) lowOut = 0;
		if(lowOut < 0.0 && lowOut > FLT_MIN_MINUS) lowOut = 0;
        
        if(highOut > 0.0 && highOut < FLT_MIN_PLUS) highOut = 0;
		if(highOut < 0.0 && highOut > FLT_MIN_MINUS) highOut = 0;
        
		// shuffle delays
		// Y delays
		m_f_Yz_2_LP = m_f_Yz_1_LP;
		m_f_Yz_1_LP = lowOut;
        
        m_f_Yz_2_HP = m_f_Yz_1_HP;
		m_f_Yz_1_HP = highOut;
        
		// X delays
		m_f_Xz_2 = m_f_Xz_1;
		m_f_Xz_1 = sample;
        
	}
    
    
private:

    void calculateFilterbankCoeffs()
    {
        double omega_c = M_PI * mFreq;
        double theta_c = M_PI * mFreq /mSampleRate;
        
        double k = omega_c/tan(theta_c);
        double k_squared = k*k;
        
        double omega_c_squared = omega_c*omega_c;
        
        double fDenominator = k_squared + omega_c_squared + 2.0*k*omega_c;
        
        double fb1_Num = -2.0*k_squared + 2.0*omega_c_squared;
        double fb2_Num = -2.0*k*omega_c + k_squared + omega_c_squared;
        
        double a0 = omega_c_squared/fDenominator;
        double a1 = 2.0*omega_c_squared/fDenominator;
        double a2 = a0;
        double b1 = fb1_Num/fDenominator;
        double b2 = fb2_Num/fDenominator;
        
        // set the LPFs
        m_f_a0_LP = a0;
        m_f_a1_LP = a1;
        m_f_a2_LP = a2;
        m_f_b1_LP = b1;
        m_f_b2_LP = b2;

        
        // the HPF coeffs
        a0 = k_squared/fDenominator;
        a1 = -2.0*k_squared/fDenominator;
        a2 = a0;
        b1 = fb1_Num/fDenominator;
        b2 = fb2_Num/fDenominator;
        
        // set the HPFs
        m_f_a0_HP = a0;
        m_f_a1_HP = a1;
        m_f_a2_HP = a2;
        m_f_b1_HP = b1;
        m_f_b2_HP = b2;
    }
    double m_f_Xz_1; // x z-1 delay element
	double m_f_Xz_2; // x z-2 delay element
    
	double m_f_Yz_1_LP; // y z-1 delay element
	double m_f_Yz_2_LP; // y z-2 delay element
    
    double m_f_Yz_1_HP; // y z-1 delay element
	double m_f_Yz_2_HP; // y z-2 delay element
    
    double mSampleRate;
    double mFreq;
};
