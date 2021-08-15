#ifndef PARTIAL_FILTER_H
#define PARTIAL_FILTER_H

#include <vector>
#include <deque>
using namespace std;

#include <Partial.h>

class PartialFilter
{
 public:
    virtual void Reset(int bufferSize) = 0;
        
    virtual void FilterPartials(vector<Partial> *partials) = 0;
};

#endif
