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

    void Split(const vector<BL_FLOAT> > &magns,
               vector<BL_FLOAT> > *sig,
               vector<BL_FLOAT> > *noise);
               
protected:
    BL_FLOAT ComputeSpectralIrreg(vector<BL_FLOAT> > &magns);

    void FindMinMax(vector<BL_FLOAT> > &values,
                    int startI, int endI,
                    BL_FLOAT *minVal, BL_FLOAT maxVal);
};

#endif
