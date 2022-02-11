//
//  Timer.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/09/17.
//
//

#ifndef Transient_Timer_h
#define Transient_Timer_h

class BlaTimer
{
public:
    BlaTimer();
    
    virtual ~BlaTimer();
    
    void Reset();
    
    unsigned long int Get();
    
protected:
    unsigned long int mPrevTime;
};

#endif
