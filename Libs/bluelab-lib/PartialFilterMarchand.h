#ifndef PARTIAL_FILTER_MARCHAND_H
#define PARTIAL_FILTER_MARCHAND_H

#include <vector>
#include <deque>
using namespace std;

#include <PartialFilter.h>

// Method of Sylvain Marchand (~2001)
class PartialFilterMarchand : public PartialFilter
{
 public:
    PartialFilterMarchand(int bufferSize, BL_FLOAT sampleRate);
    virtual ~PartialFilterMarchand();

    void Reset(int bufferSize, BL_FLOAT sampleRate);
        
    void FilterPartials(vector<Partial> *partials);

 protected:
    // Simple method, based on frequencies only
    void AssociatePartials(const vector<Partial> &prevPartials,
                           vector<Partial> *currentPartials,
                           vector<Partial> *remainingPartials);
    
    // See: https://www.dsprelated.com/freebooks/sasp/PARSHL_Program.html#app:parshlapp
    // "Peak Matching (Step 5)"
    // Use fight/winner/loser
    void AssociatePartialsPARSHL(const vector<Partial> &prevPartials,
                                 vector<Partial> *currentPartials,
                                 vector<Partial> *remainingPartials);

    BL_FLOAT GetDeltaFreqCoeff(int binNum);

    int FindPartialById(const vector<Partial> &partials, int idx);
    
    //
    deque<vector<Partial> > mPartials;

    int mBufferSize;
    
 private:
    vector<Partial> mTmpPartials0;
    vector<Partial> mTmpPartials1;
    vector<Partial> mTmpPartials2;
    vector<Partial> mTmpPartials3;
    vector<Partial> mTmpPartials4;
    vector<Partial> mTmpPartials5;
    vector<Partial> mTmpPartials6;
};

#endif
