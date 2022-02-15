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
    
protected:
    BL_FLOAT computeSpectralIrreg(const vector<BL_FLOAT> &magns,
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
};

#endif
