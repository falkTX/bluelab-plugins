//
//  USTEarlyReflect.cpp
//  UST
//
//  Created by applematuer on 12/3/19.
//
//

#include "revmodel.hpp"

#include "USTEarlyReflect.h"


USTEarlyReflect::USTEarlyReflect(double sampleRate)
{
    mRevModel = new revmodel();
    
    mRevModel->init(sampleRate);
    InitRevModel();
}

USTEarlyReflect::~USTEarlyReflect()
{
    delete mRevModel;
}

void
USTEarlyReflect::Reset(double sampleRate)
{
    // Re-create, otherwise we have nan with 88200Hz sample rate
    //delete mRevModel;
    //mRevModel = new revmodel();
    
    mRevModel->init(sampleRate);
    InitRevModel();
}

void
USTEarlyReflect::Process(const WDL_TypedBuf<double> &input,
                         WDL_TypedBuf<double> *outputL,
                         WDL_TypedBuf<double> *outputR,
                         double revGain)
{
    outputL->Resize(input.GetSize());
    outputR->Resize(input.GetSize());
    
    for (int i = 0; i < input.GetSize(); i++)
    {
        double in = input.Get()[i];
        double outL;
        double outR;
        
        mRevModel->process(in, outL, outR);
        
        outputL->Get()[i] = outL*revGain;
        outputR->Get()[i] = outR*revGain;
    }
}

void
USTEarlyReflect::InitRevModel()
{
    //mRevModel->setroomsize(0.5);
    mRevModel->setroomsize(0.1);
    //mRevModel->setdamp(0.5);
    mRevModel->setdamp(2.0);
    mRevModel->setwet(0.3);
    mRevModel->setdry(0.0);
    mRevModel->setwidth(1.0);
    mRevModel->setmode(0.0);
}
