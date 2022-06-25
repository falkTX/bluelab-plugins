/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
#ifndef IMPULSE_RESPONSE_SET_H
#define IMPULSE_RESPONSE_SET_H

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

#define ALIGN_IR_ABSOLUTE 1
#define ALIGN_IR_TIME 0.004

// The set of captured impulse responses
// Can make some processing and some statistics
class ImpulseResponseSet
{
 public:
    ImpulseResponseSet(long respSize, BL_FLOAT sampleRate);
    
    virtual ~ImpulseResponseSet();
  
    void Reset(long respSize, BL_FLOAT sampleRate);
  
    void SetSampleRate(BL_FLOAT sampleRate);
  
    void AddImpulseResponse(const WDL_TypedBuf<BL_FLOAT> &impulseResponse);
  
    // Get the last added impulse response
    // (which has been aligned !)
    void GetLastImpulseResponse(WDL_TypedBuf<BL_FLOAT> *impulseResponse);
  
    void GetAvgImpulseResponse(WDL_TypedBuf<BL_FLOAT> *impulseResponse);
  
    // Empty the set
    void Clear();
  
    long GetSize() const;
  
    static void AlignImpulseResponse(WDL_TypedBuf<BL_FLOAT> *response,
                                     long responseSize, BL_FLOAT decimFactor,
                                     BL_FLOAT sampleRate);
  
    static void AlignImpulseResponses(WDL_TypedBuf<BL_FLOAT> responses[2],
                                      long responseSize,
                                      BL_FLOAT sampleRate);
  
    static void NormalizeImpulseResponses(WDL_TypedBuf<BL_FLOAT> responses[2]);
  
    static void AlignImpulseResponsesAll(ImpulseResponseSet *impRespSets[2],
                                         long responseSize,
                                         BL_FLOAT sampleRate);
  
    static void AlignImpulseResponsesAll(ImpulseResponseSet *impRespSet,
                                         long responseSize,
                                         BL_FLOAT decimationFactor,
                                         BL_FLOAT sampleRate);
  
    static void AlignImpulseResponseSamples(WDL_TypedBuf<BL_FLOAT> *response,
                                            long responseSize,
                                            BL_FLOAT sampleRate);

  
    static void DiscardBadResponses(ImpulseResponseSet *responseSet,
                                    bool *lastRespValid, bool iterate);
  
    static void DiscardBadResponses(ImpulseResponseSet *responseSets[2],
                                    bool *lastRespValid, bool iterate);
  
    // Resize if necessary, then make fades
    static void MakeFades(WDL_TypedBuf<BL_FLOAT> *impulseResponse, long maxSize,
                          BL_FLOAT decimFactor);
  
    // For a mono response
    static void NormalizeImpulseResponse(WDL_TypedBuf<BL_FLOAT> *impulseResponse);
  
    // For a pair of responses
    static void NormalizeImpulseResponse2(WDL_TypedBuf<BL_FLOAT> *impulseResponse0,
                                          WDL_TypedBuf<BL_FLOAT> *impulseResponse1);
  
    static void DenoiseImpulseResponse(WDL_TypedBuf<BL_FLOAT> *impulseResponse);
  
    // Utility
    static long FindMaxValIndex(const WDL_TypedBuf<BL_FLOAT> &buf, int maxIndexSearch = -1);
  
 protected:
    void DiscardBadImpulseResponses(vector<long> *respToDiscard, long discardMaxSamples);
  
    void RemoveResponses(const vector<long> &indicesToRemove);
  
    // Align the new impulse response to the previous ones
    static void AlignImpulseResponse(WDL_TypedBuf<BL_FLOAT> *impulseResponse, long indexForMax);
  
    static void AlignImpulseResponse2(WDL_TypedBuf<BL_FLOAT> *impulseResponse0,
                                      WDL_TypedBuf<BL_FLOAT> *impulseResponse1,
                                      long indexForMax);
  
    static bool DiscardBadImpulseResponses(ImpulseResponseSet *respSet,
                                           bool *lastRespValid,
                                           long discardMaxSamples, bool iterate);
  
    static bool DiscardBadImpulseResponses2(ImpulseResponseSet *respSet0,
                                            ImpulseResponseSet *respSet1,
                                            bool *lastRespValid,
                                            long discardMaxSamples, bool iterate);
  
    // Utilities
    static BL_FLOAT GetMax(const WDL_TypedBuf<BL_FLOAT> &buf);

    static void MultImpulseResponse(WDL_TypedBuf<BL_FLOAT> *impulseResponse, BL_FLOAT val);
  
    static BL_FLOAT ComputeSigma(const WDL_TypedBuf<BL_FLOAT> &impulseResponse,
                                 const WDL_TypedBuf<BL_FLOAT> &avgResponse,
                                 long maxSamples);
  
    static bool CheckLastRespValid(const ImpulseResponseSet *respSet,
                                   const vector<long> &indicesToDiscard);

  
    //
    long mResponseSize;
  
    vector<WDL_TypedBuf<BL_FLOAT> > mResponses;
  
    BL_FLOAT mSampleRate;
};

#endif
