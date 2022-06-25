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
 
#ifndef BL_UTILS_PLUG_H
#define BL_UTILS_PLUG_H

#include <vector>
using namespace std;

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;

class ParamSmoother2;
class BLUtilsPlug
{
 public:
    BLUtilsPlug();
    virtual ~BLUtilsPlug();
    
    // Plugin operations
    static void PlugInits();

    static void SetPlugResizable(Plugin *plug, bool flag);
    
    static void BypassPlug(double **inputs, double **outputs, int nFrames);
    
    // Get all the buffers available to the plugin
    static void GetPlugIOBuffers(Plugin *plug, double **inputs, double **outputs,
                                 double *inp[2], double *scIn[2], double *outp[2]);
    
    static bool GetPlugIOBuffers(Plugin *plug,
                                 double **inputs, double **outputs, int nFrames,
                                 vector<WDL_TypedBuf<BL_FLOAT> > *inp,
                                 vector<WDL_TypedBuf<BL_FLOAT> > *scIn,
                                 vector<WDL_TypedBuf<BL_FLOAT> > *outp);

    static void GetPlugNumIO(Plugin *plug,
                             const double **inputs, const double **outputs,
                             int *nIn, int *nOut);
        
    // Touch plug param for automation to be written
    static void TouchPlugParam(Plugin *plug, int paramIdx);

    // Set non normalized value
    static void SetParameterValue(Plugin *plug, int paramIdx, BL_FLOAT nonNormValue,
                                  bool updateControl);

    static void ResetParameterValue(Plugin *plug, int paramIdx, bool updateControl);
    
    // For a give index, try to get both in and out buffers
    static bool GetIOBuffers(int index, double *in[2], double *out[2],
                             double **inBuf, double **outBuf);
    
    static bool GetIOBuffers(int index,
                             vector<WDL_TypedBuf<double> > &in,
                             vector<WDL_TypedBuf<double> > &out,
                             double **inBuf, double **outBuf);
    
    static bool GetIOBuffers(int index,
                             vector<WDL_TypedBuf<double> > &in,
                             vector<WDL_TypedBuf<double> > &out,
                             WDL_TypedBuf<double> **inBuf,
                             WDL_TypedBuf<double> **outBuf);

    static bool PlugIOAllZero(double *inputs[2], double *outputs[2], int nFrames);
    
    static bool PlugIOAllZero(const vector<WDL_TypedBuf<BL_FLOAT> > &inputs,
                              const vector<WDL_TypedBuf<BL_FLOAT> > &outputs);
    
    static void PlugCopyOutputs(const vector<WDL_TypedBuf<BL_FLOAT> > &outp,
                                double **outputs, int nFrames);
    
    static int PlugComputeBufferSize(int bufferSize, BL_FLOAT sampleRate);
    
    static void PlugUpdateLatency(Plugin *plug,
                                  int nativeBufferSize, int nativeLatency,
                                  BL_FLOAT sampleRate);

    static int PlugComputeLatency(Plugin *plug,
                                  int nativeBufferSize, int nativeLatency,
                                  BL_FLOAT sampleRate);
    
    static BL_FLOAT GetBufferSizeCoeff(Plugin *plug, int nativeBufferSize);

    static bool ChannelAllZero(const vector<WDL_TypedBuf<BL_FLOAT> > &channel);

    static bool GetFullPlugResourcesPath(const IPluginBase &plug,
                                         WDL_String *resPath);

    static WDL_TypedBuf<uint8_t> LoadWinResource(const char *resourceName,
                                                 const char *type);
    
    static void ApplyGain(const vector<WDL_TypedBuf<BL_FLOAT> > &in,
                          vector<WDL_TypedBuf<BL_FLOAT> > *out,
                          ParamSmoother2 *smoother);

    static void ApplyDryWet(const vector<WDL_TypedBuf<BL_FLOAT> > &in,
                            vector<WDL_TypedBuf<BL_FLOAT> > *out,
                            ParamSmoother2 *smoother);
    
    static void SumSignals(const vector<WDL_TypedBuf<BL_FLOAT> > &a,
                           const vector<WDL_TypedBuf<BL_FLOAT> > &b,
                           vector<WDL_TypedBuf<BL_FLOAT> > *out);
                           
    static int GetPlugFPS(int defaultFPS);

    static BL_FLOAT GetTransportTime(Plugin *plug);

    // Buf must have been allocated before
    static void ChannelsToInterleaved(const vector<WDL_TypedBuf<BL_FLOAT> > &chans,
                                      BL_FLOAT *buf);

    // chans must have been resized correctly before
    static void InterleavedToChannels(const BL_FLOAT *buf,
                                      vector<WDL_TypedBuf<BL_FLOAT> > *chans);

    // Some hosts, like Bitwig/linux/vst2 does not call OnReset() when playback starts
    void CheckReset(Plugin *plug);

protected:
    bool mWasPlaying;
};

#endif
