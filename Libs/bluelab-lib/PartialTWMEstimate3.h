//
//  PartialTWMEstimate3.h
//  BL-SASViewer
//
//  Created by applematuer on 2/8/19.
//
//

#ifndef __BL_SASViewer__PartialTWMEstimate3__
#define __BL_SASViewer__PartialTWMEstimate3__

#include <vector>
using namespace std;

#include <PartialTracker5.h>


// Estimate fundamental frequency from series of partials (more or less harmonic)
// (works well for artifacts and reverb)
//
// See: https://pdfs.semanticscholar.org/c94b/56f21f32b3b7a9575ced317e3de9b2ad9081.pdf
//
// and: https://pdfs.semanticscholar.org/5c39/c1a6c13b8ee7a4947d150267b803e6aa5ab6.pdf
//
class PartialTWMEstimate3
{
public:
    class Freq
    {
    public:
        static bool ErrorLess(const Freq &f1, const Freq &f2);
        
        BL_FLOAT mFreq;
        BL_FLOAT mError;
    };
    
    PartialTWMEstimate3(int bufferSize, BL_FLOAT sampleRate);
    
    virtual ~PartialTWMEstimate3();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetHarmonicSoundFlag(bool flag);
    
    // Naive implementation, just for testing
    BL_FLOAT EstimateNaive(const vector<PartialTracker5::Partial> &partials);
    
    // Estimate the fundamental frequency from a series of partials
    BL_FLOAT Estimate(const vector<PartialTracker5::Partial> &partials);
    
    // Same as above, but optimized: start with rought precision then
    // increase around the interesting frequency
    //
    // The best method
    BL_FLOAT EstimateMultiRes(const vector<PartialTracker5::Partial> &partials);
    
    // See optimization in (4.2)
    // here: https://pdfs.semanticscholar.org/5c39/c1a6c13b8ee7a4947d150267b803e6aa5ab6.pdf
    BL_FLOAT EstimateOptim(const vector<PartialTracker5::Partial> &partials);
    
    // Compute al the intervals, and find the frequency from these intervals
    //
    // Very good performances, but some jumps
    BL_FLOAT EstimateOptim2(const vector<PartialTracker5::Partial> &partials);
    
protected:
    BL_FLOAT Estimate(const vector<PartialTracker5::Partial> &partials,
                    BL_FLOAT freqAccuracy,
                    BL_FLOAT minFreqSearch, BL_FLOAT maxFreqSearch,
                    BL_FLOAT maxFreqHarmo,
                    BL_FLOAT *error = NULL);
    
    void EstimateMulti(const vector<PartialTracker5::Partial> &partials,
                       BL_FLOAT freqAccuracy,
                       BL_FLOAT minFreqSearch, BL_FLOAT maxFreqSearch,
                       BL_FLOAT maxFreqHarmo,
                       vector<Freq> *freqs);
    
    BL_FLOAT ComputeTWMError(const vector<PartialTracker5::Partial> &partials,
                           BL_FLOAT testFreq, BL_FLOAT maxFreqHarmo, BL_FLOAT AmaxInv,
                           vector<BL_FLOAT> *dbgHarmos = NULL,
                           BL_FLOAT *dbgErrorK = NULL,
                           BL_FLOAT *dbgErrorN = NULL);
    
    // Optimized find
    BL_FLOAT ComputeTWMError2(const vector<PartialTracker5::Partial> &partials,
                            const vector<BL_FLOAT> &partialFreqs,
                            BL_FLOAT testFreq, BL_FLOAT maxFreqHarmo, BL_FLOAT AmaxInv);
    
    BL_FLOAT ComputeErrorK(const PartialTracker5::Partial &partial,
                         BL_FLOAT harmo, BL_FLOAT AmaxInv);
    
    BL_FLOAT ComputeErrorN(const PartialTracker5::Partial &partial,
                         BL_FLOAT harmo, BL_FLOAT AmaxInv);
    
    // Optimized find + coeffs precomputation
    BL_FLOAT ComputeTWMError3(const vector<PartialTracker5::Partial> &partials,
                            const vector<BL_FLOAT> &partialFreqs,
                            BL_FLOAT testFreq, BL_FLOAT maxFreqHarmo,
                            const vector<BL_FLOAT> &aNorms,
                            const vector<BL_FLOAT> &fkps);
    
    BL_FLOAT ComputeErrorK2(const PartialTracker5::Partial &partial,
                          BL_FLOAT harmo, BL_FLOAT aNorm,
                          BL_FLOAT fkp);
    
    BL_FLOAT ComputeErrorN2(const PartialTracker5::Partial &partial,
                          BL_FLOAT harmo, BL_FLOAT aNorm);

    
    //
    // Max freq for harmonics generation
    BL_FLOAT FindMaxFreqHarmo(const vector<PartialTracker5::Partial> &partials);

    // Max for the search range of F0
    BL_FLOAT FindMaxFreqSearch(const vector<PartialTracker5::Partial> &partials);

    //
    void SelectPartials(vector<PartialTracker5::Partial> *partials);
    void GetAlivePartials(vector<PartialTracker5::Partial> *partials);
    void SuppressNewPartials(vector<PartialTracker5::Partial> *partials);
    bool FindPartialById(const vector<PartialTracker5::Partial> &partials, int idx);
    
    // Optim
    int FindNearestIndex(const vector<BL_FLOAT> &freqs, BL_FLOAT freq);
    void LimitPartialsNumber(vector<PartialTracker5::Partial> *sortedPartials);

    
    //
    int mBufferSize;
    BL_FLOAT mSampleRate;
    
    // Set to true for clear voice, and to false for similar to bell
    bool mHarmonicSoundFlag;
    
    vector<PartialTracker5::Partial> mPrevPartials;
};

#endif /* defined(__BL_SASViewer__PartialTWMEstimate3__) */
