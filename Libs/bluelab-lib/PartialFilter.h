#ifndef PARTIAL_FILTER_H
#define PARTIAL_FILTER_H

#include <vector>
#include <deque>
using namespace std;

#include <Partial.h>

// Method of Sylvain Marchand (~2001)
class PartialFilter
{
 public:
    PartialFilter(int bufferSize);
    virtual ~PartialFilter();

    void Reset(int bufferSize);
        
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
    vector<Partial> mTmpPartials12;
    vector<Partial> mTmpPartials13;
    vector<Partial> mTmpPartials14;
    vector<Partial> mTmpPartials15;
    vector<Partial> mTmpPartials16;
    vector<Partial> mTmpPartials17;
    vector<Partial> mTmpPartials18;
};

#endif
