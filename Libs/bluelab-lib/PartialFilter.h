#ifndef PARTIAL_FILTER_H
#define PARTIAL_FILTER_H

#include <vector>
using namespace std;

#include <Partial.h>

// Method of Sylvain Marchand (~2001)
class PartialFilter
{
 public:
    PartialFilter();
    virtual ~PartialFilter();
    
    void FilterPartials(vector<Partial> *partials);
};

#endif
