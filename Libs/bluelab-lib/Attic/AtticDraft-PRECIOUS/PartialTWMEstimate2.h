//
//  PartialTWMEstimate2.h
//  BL-SASViewer
//
//  Created by applematuer on 2/8/19.
//
//

#ifndef __BL_SASViewer__PartialTWMEstimate2__
#define __BL_SASViewer__PartialTWMEstimate2__

#include <vector>
using namespace std;

#include <PartialTracker3.h>


// Estimate fundamental frequency from series of partials (more or less harmonic)
// (works well for artifacts and reverb)
//
// See: https://pdfs.semanticscholar.org/c94b/56f21f32b3b7a9575ced317e3de9b2ad9081.pdf
//
// and: https://pdfs.semanticscholar.org/5c39/c1a6c13b8ee7a4947d150267b803e6aa5ab6.pdf
//
class PartialTWMEstimate2
{
public:
    PartialTWMEstimate2(int bufferSize, double sampleRate);
    
    virtual ~PartialTWMEstimate2();
    
    void Reset(double sampleRate);
    
    // Estimate the fundamental frequency from a series of partials
    double Estimate(const vector<PartialTracker3::Partial> &partials);
    
    // Same as above, but optimized: start with rought precision then
    // increase around the interesting frequency
    double EstimateMultiRes(const vector<PartialTracker3::Partial> &partials);
    
    // See optimization in (4.2)
    // here: https://pdfs.semanticscholar.org/5c39/c1a6c13b8ee7a4947d150267b803e6aa5ab6.pdf
    double EstimateOptim(const vector<PartialTracker3::Partial> &partials);
    
    
    // Fix big freqs jumps (TEST)
    double FixFreqJumps(double freq0, double prevFreq);
    
    // Gest nearest hamonic from input freq, comared to reference freq
    double GetNearestOctave(double freq, double refFreq);
    
    // Gest nearest partial from input freq, comared to reference freq
    double GetNearestHarmonic(double freq, double refFreq);
    
protected:
    double Estimate(const vector<PartialTracker3::Partial> &partials,
                    double freqAccuracy,
                    double minFreq, double maxFreq,
                    double *error = NULL);
    
    double ComputeTWMError(const vector<PartialTracker3::Partial> &partials,
                           double testFreq, double maxFreq, double AmaxInv,
                           vector<double> *dbgHarmos = NULL);
    
    double ComputeErrorK(const PartialTracker3::Partial &partial,
                         double harmo, double AmaxInv);
    
    double ComputeErrorN(const PartialTracker3::Partial &partial,
                         double harmo, double AmaxInv);
    
    //
    // Max freq for harmonics generation
    double FindMaxFreqHarmo(const vector<PartialTracker3::Partial> &partials);

    // Max for the search range of F0
    double FindMaxFreqSearch(const vector<PartialTracker3::Partial> &partials);

    
    void PartialsRange(vector<PartialTracker3::Partial> *partials,
                       double minFreq, double maxFreq);

    //
    void DBG_DumpFreqs(const char *fileName,
                       const vector<double> &freqs);

    
    //
    int mBufferSize;
    double mSampleRate;
};

#endif /* defined(__BL_SASViewer__PartialTWMEstimate2__) */
