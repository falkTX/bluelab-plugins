#include <BLUtilsMath.h>

#include "QIFFT.h"

void
QIFFT::FindPeak(const WDL_TypedBuf<BL_FLOAT> &magns,
                const WDL_TypedBuf<BL_FLOAT> &phases,
                int peakBin, Peak *result)
{
    // Default value
    result->mBinIdx = peakBin;
    result->mAmp = magns.Get()[peakBin];
    result->mPhase = phases.Get()[peakBin];
    result->mAlpha0 = 0.0;
    result->mBeta0 = 0.0;
        
    if ((peakBin - 1 < 0) || (peakBin + 1 >= magns.GetSize()))
        return;
    
    // Magns are in DB
    BL_FLOAT alpha = magns.Get()[peakBin - 1];
    BL_FLOAT beta = magns.Get()[peakBin];
    BL_FLOAT gamma = magns.Get()[peakBin + 1];

    // Will avoid wrong negative result
    if ((beta < alpha) || (beta < gamma))
        return;

    // Center
    BL_FLOAT denom = (alpha - 2.0*beta + gamma);
    if (std::fabs(denom) < BL_EPS)
        return;
    
    BL_FLOAT c = 0.5*((alpha - gamma)/denom);

    // We have the true bin!
    result->mBinIdx = peakBin + c;

    // We have the true amp!
    result->mAmp = beta - 0.25*(alpha - gamma)*c;

    // Phases
    BL_FLOAT alphaP = phases.Get()[peakBin - 1];
    BL_FLOAT betaP = phases.Get()[peakBin];
    BL_FLOAT gammaP = phases.Get()[peakBin + 1];
    
    // We have the true phase!
    // (should we unwrap phases before ?)
    result->mPhase = betaP - 0.25*(alphaP - gammaP)*c;

    // For alpha0 and beta0,
    // See: https://ccrma.stanford.edu/files/papers/stanm118.pdf

    // Eps used for derivative
#define DERIV_EPS 1e-10

    //y(c) = beta - 0.25*(alpha - gamma)*c

    // Magnitudes
    //
    
    // Magnitude derivative at mBinIdx
    BL_FLOAT a0 = beta - 0.25*(alpha - gamma)*(c - DERIV_EPS);
    BL_FLOAT a1 = beta - 0.25*(alpha - gamma)*(c + DERIV_EPS);
    BL_FLOAT vp = (a1 - a0)/(2.0*DERIV_EPS);

    // Second derivative of magnitudes
    //

    // Derivative at n - 1
    BL_FLOAT a00 = beta - 0.25*(alpha - gamma)*(c - 2.0*DERIV_EPS);
    BL_FLOAT a10 = beta - 0.25*(alpha - gamma)*c;
    BL_FLOAT vp0 = (a10 - a00)/(2.0*DERIV_EPS);

    // Derivative at n + 1
    BL_FLOAT a01 = beta - 0.25*(alpha - gamma)*c;
    BL_FLOAT a11 = beta - 0.25*(alpha - gamma)*(c + 2.0*DERIV_EPS);
    BL_FLOAT vp1 = (a10 - a00)/(2.0*DERIV_EPS);

    // Second derivative
    BL_FLOAT vpp = (vp1 - vp0)/(2.0*DERIV_EPS);

    // Phases
    //
    
    // Phases derivative at mBinIdx
    BL_FLOAT p0 = betaP - 0.25*(alphaP - gammaP)*(c - DERIV_EPS);
    BL_FLOAT p1 = betaP - 0.25*(alphaP - gammaP)*(c + DERIV_EPS);
    BL_FLOAT up = (a1 - a0)/(2.0*DERIV_EPS);

    // Second derivative of phases
    //

    // Derivative at n - 1
    BL_FLOAT p00 = betaP - 0.25*(alphaP - gammaP)*(c - 2.0*DERIV_EPS);
    BL_FLOAT p10 = betaP - 0.25*(alphaP - gammaP)*c;
    BL_FLOAT up0 = (p10 - p00)/(2.0*DERIV_EPS);

    // Derivative at n + 1
    BL_FLOAT p01 = betaP - 0.25*(alphaP - gammaP)*c;
    BL_FLOAT p11 = betaP - 0.25*(alphaP - gammaP)*(c + 2.0*DERIV_EPS);
    BL_FLOAT up1 = (p10 - p00)/(2.0*DERIV_EPS);

    // Second derivative
    BL_FLOAT upp = (up1 - up0)/(2.0*DERIV_EPS);

    // Then finally compute alpha0 and beta0
    //
    BL_FLOAT p = -upp/(2.0*(upp*upp + vpp*vpp));
    BL_FLOAT alpha0 = -2.0*p*vp;
    BL_FLOAT beta0 = p*vpp/upp;

    // Result
    result->mAlpha0 = alpha0;
    result->mBeta0 = beta0;
}
