#ifndef SI_SPLITTER_H
#define SI_SPLITTER_H

#include <vector>
using namespace std;

#include <BLTypes.h>

// QUESTION: log freqs? / log magns?
// IDEA: use Marquardt instead?
class SISplitter
{
public:
    SISplitter();
    virtual ~SISplitter();

    void split(const vector<BL_FLOAT> &magns,
               vector<BL_FLOAT> *sig,
               vector<BL_FLOAT> *noise);
    
    void setOffset(float offset);

    void setResolution(int reso);
        
protected:
    // Normalized spectral irregularity - Jensen, 1999
    BL_FLOAT computeSpectralIrreg_J(const vector<BL_FLOAT> &magns,
                                    int startBin, int endBin);
    // Spectral irregularity - Krimphoff et al., 1994
    BL_FLOAT computeSpectralIrreg_K(const vector<BL_FLOAT> &magns,
                                    int startBin, int endBin);
    
    void computeSpectralIrregWin(const vector<BL_FLOAT> &magns,
                                 vector<BL_FLOAT> *siWin,
                                 int winSize, int overlap);
    
    BL_FLOAT computeScore(const vector<BL_FLOAT> &magns,
                          BL_FLOAT refMagn,
                          int startBin, int endBin);
        
    void findMinMax(const vector<BL_FLOAT> &values,
                    int startBin, int endBin,
                    BL_FLOAT *minVal, BL_FLOAT *maxVal);

    //
    float _offset;

    int _reso;
};

#endif
