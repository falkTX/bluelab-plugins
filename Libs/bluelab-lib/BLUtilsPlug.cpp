#define INI_IMPLEMENTATION
#include <ini.h>

#include <IPlugPaths.h>

#include <BLTypes.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>
#include <BLUtilsFile.h>

#include <ParamSmoother2.h>

#include "IPlug_include_in_plug_hdr.h"

#include "BLUtilsPlug.h"

using namespace iplug;

#define TRY_FIX_SIDE_CHAIN_AU 1

void
BLUtilsPlug::PlugInits()
{
    // Necessary on Linux. Otherwise with the APP version of plug,
    // locale could be another country such as FR, and for example
    // the digit separation would be ',' instad of ".".
    // This would make fail functions such as atof() e.g
    // and this would show a ',' in the knob values instead of a '.'
#if __linux__
#ifdef APP_API
    setlocale(LC_ALL, "C");
#endif
#endif
}

void
BLUtilsPlug::BypassPlug(double **inputs, double **outputs, int nFrames)
{
    if ((inputs[0] != NULL) && (outputs[0] != NULL))
        memcpy(outputs[0], inputs[0], nFrames*sizeof(double));

    if ((inputs[1] != NULL) && (outputs[1] != NULL))
        memcpy(outputs[1], inputs[1], nFrames*sizeof(double));
}

// #bl-iplug2: ok
#if 0
void
BLUtilsPlug::GetPlugIOBuffers(Plugin *plug, double **inputs, double **outputs,
                              double *in[2], double *scIn[2], double *out[2])
{
    int numInChannels = plug->NInChannels();
    int numInScChannels = plug->NInScChannels();
    
#ifdef AAX_API
    // Protools only takes one channel for side chain
    
    // force it to 1, just in case
    if (numInScChannels > 1)
        numInScChannels = 1;
#endif
    
    bool isInConnected[4] = { false, false, false, false };
    
    for (int i = 0; i < numInChannels; i++)
    {
        isInConnected[i] = plug->IsInChannelConnected(i);
    }
    
    // in
    in[0] = ((numInChannels - numInScChannels > 0) &&
             isInConnected[0]) ? inputs[0] : NULL;
    in[1] = ((numInChannels - numInScChannels >  1) &&
             isInConnected[1]) ? inputs[1] : NULL;
    
#ifndef APP_API
    // scin
    scIn[0] = ((numInScChannels > 0) &&
               isInConnected[numInChannels - numInScChannels]) ?
        inputs[numInChannels - numInScChannels] : NULL;
    scIn[1] = ((numInScChannels > 1) &&
               isInConnected[numInChannels - numInScChannels + 1]) ?
        inputs[numInChannels - numInScChannels + 1] : NULL;
#else
    // When in application mode, must deactivate sidechains
    // BUG: otherwise it crashes if we try to get sidechains
    scIn[0] = NULL;
    scIn[1] = NULL;
#endif
    
    // out
    out[0] = plug->IsOutChannelConnected(0) ? outputs[0] : NULL;
    out[1] = plug->IsOutChannelConnected(1) ? outputs[1] : NULL;
}
#endif

//#bl-iplug2
void
BLUtilsPlug::GetPlugIOBuffers(Plugin *plug, double **inputs, double **outputs,
                              double *in[2], double *scIn[2], double *out[2])
{
    // TODO: manage sidechanin
    // And also chacke AAX_API
    for (int i = 0; i < 2; i++)
        scIn[i] = NULL;
    
    // Inputs
    for (int i = 0; i < 2; i++)
    {
        in[i] = NULL;
        if (plug->IsChannelConnected(kInput, i) &&
            (inputs[i] != NULL))
        {
            in[i] = inputs[i];
        }
    }
            
    // Outputs
    for (int i = 0; i < 2; i++)
    {
        out[i] = NULL;
        
        if (plug->IsChannelConnected(kOutput, i) &&
            (outputs[i] != NULL))
        {
            out[i] = outputs[i];
        }
    }
}

#if 0 // Prev, dosn't manage sidechain
//#bl-iplug2
void
BLUtilsPlug::GetPlugIOBuffers(Plugin *plug,
                              double **inputs, double **outputs, int nFrames,
                              vector<WDL_TypedBuf<BL_FLOAT> > *inp,
                              vector<WDL_TypedBuf<BL_FLOAT> > *scIn,
                              vector<WDL_TypedBuf<BL_FLOAT> > *outp)
{
    // TODO: manage sidechain
    // And also check side chain AAX_API
    scIn->resize(0);
    
    // Inputs
    int numInChannelsConnected = plug->NInChansConnected();
    
    // #bl-iplug2 HACK
    if (numInChannelsConnected > 2)
        numInChannelsConnected = 2;
    
    inp->resize(numInChannelsConnected);
    for (int i = 0; i < inp->size(); i++)
    {
        if (plug->IsChannelConnected(kInput, i))
        {
            WDL_TypedBuf<BL_FLOAT> buf;
#if !BL_TYPE_FLOAT
            buf.Add(inputs[i], nFrames);
            
#else
            //BLUtilsPlug::ConvertToFloatType(&buf, inputs[i]);
            buf.Resize(nFrames);
            for (int j = 0; j < nFrames; j++)
                buf.Get()[j] = inputs[i][j];
#endif
            
            (*inp)[i] = buf;
        }
    }
    
    // Outputs
    int numOutChannelsConnected = plug->NOutChansConnected();
    outp->resize(numOutChannelsConnected);
    for (int i = 0; i < outp->size(); i++)
    {
        if (plug->IsChannelConnected(kOutput, i))
        {
            WDL_TypedBuf<BL_FLOAT> buf;
            
#if !BL_TYPE_FLOAT
            buf.Add(outputs[i], nFrames);
#else
            buf.Resize(nFrames);
            for (int j = 0; j < nFrames; j++)
                buf.Get()[j] = outputs[i][j];
#endif
            
            (*outp)[i] = buf;
        }
    }
}
#endif

// iPlug2
bool
BLUtilsPlug::GetPlugIOBuffers(Plugin *plug,
                              sample **inputs, sample **outputs, int nFrames,
                              vector<WDL_TypedBuf<BL_FLOAT> > *inp,
                              vector<WDL_TypedBuf<BL_FLOAT> > *scInp,
                              vector<WDL_TypedBuf<BL_FLOAT> > *outp)
{
#define MAX_NUM_IN_CHANNELS 4
#define MAX_NUM_OUT_CHANNELS 2
    
    /*
     
      Logic/Garageband have an long-standing bug where if no sidechain is selected, the same buffers that are sent to the first bus, are sent to the sidechain bus
      https://forum.juce.com/t/sidechain-is-not-silent-as-expected-au-logic-x-10-2-2/17068/8
      https://lists.apple.com/archives/coreaudio-api/2012/Feb/msg00127.html
     
      Imperfect hack around it here. Probably a better solution is to have an enable sidechain button in the plug-in UI, in addition to the host sidechain routing.
    */
    
#if defined OS_MAC && defined AU_API
    if(plug->GetHost() == kHostLogic || plug->GetHost() == kHostGarageBand)
    {
        const int sz = nFrames * sizeof(sample);
        if(!memcmp(inputs[0], inputs[2], sz))
        {
            memset(inputs[2], 0, sz);
            inputs[2] = NULL;
        }
        if(!memcmp(inputs[1], inputs[3], sz))
        {
            memset(inputs[3], 0, sz);
            inputs[3] = NULL;
        }
    }
#endif

    // Compute the number of inputs and outputs
    int numIn = 0;
    for (int i = 0; i < MAX_NUM_IN_CHANNELS; i++)
    {
        bool connected = plug->IsChannelConnected(ERoute::kInput, i);
        if(connected && (inputs[i] != NULL))
            numIn++;
    }

    int numScIn = numIn - 2;
    if (numScIn < 0)
        numScIn = 0;

    int numOut = 0;
    for (int i = 0; i < MAX_NUM_OUT_CHANNELS; i++)
    {
        bool connected = plug->IsChannelConnected(ERoute::kOutput, i);
        if(connected && (outputs[i] != NULL))
            numOut++;
    }

    if ((numIn == 0) &&
        (numScIn == 0) &&
        (numOut == 0))
        return false;
    
    // Resize the buffers if necessary
    inp->resize(numIn);
    scInp->resize(numScIn);
    outp->resize(numOut);

    // Fill the buffers
    for (int i = 0; i < MAX_NUM_IN_CHANNELS; i++)
    {
        bool connected = plug->IsChannelConnected(ERoute::kInput, i);
        if(connected && (inputs[i] != NULL))
        {
            if (i < 2)
                // input
            {                
                WDL_TypedBuf<BL_FLOAT> &buf = (*inp)[i];
#if !BL_TYPE_FLOAT
                buf.Resize(nFrames);
                memcpy(buf.Get(), inputs[i], nFrames*sizeof(BL_FLOAT));
#else
                buf.Resize(nFrames);
                for (int j = 0; j < nFrames; j++)
                    buf.Get()[j] = inputs[i][j];
#endif
            }
            else
            {          
                WDL_TypedBuf<BL_FLOAT> &buf = (*scInp)[i - 2];
#if !BL_TYPE_FLOAT
                buf.Resize(nFrames);
                memcpy(buf.Get(), inputs[i], nFrames*sizeof(BL_FLOAT));
#else
                buf.Resize(nFrames);
                for (int j = 0; j < nFrames; j++)
                    buf.Get()[j] = inputs[i][j];
#endif
            }
        }
    }
    
    for (int i = 0; i < MAX_NUM_OUT_CHANNELS; i++)
    {
        bool connected = plug->IsChannelConnected(ERoute::kOutput, i);
        if(connected && (outputs[i] != NULL))
        {      
            WDL_TypedBuf<BL_FLOAT> &buf = (*outp)[i];
#if !BL_TYPE_FLOAT
            buf.Resize(nFrames);
            memcpy(buf.Get(), outputs[i], nFrames*sizeof(BL_FLOAT));
#else
            buf.Resize(nFrames);
            for (int j = 0; j < nFrames; j++)
                buf.Get()[j] = outputs[i][j];
#endif
        }
    }

    return true;
}

// #bl-iplug2: ok
#if 0
void
BLUtilsPlug::GetPlugIOBuffers(Plugin *plug,
                              double **inputs, double **outputs, int nFrames,
                              vector<WDL_TypedBuf<BL_FLOAT> > *inp,
                              vector<WDL_TypedBuf<BL_FLOAT> > *scIn,
                              vector<WDL_TypedBuf<BL_FLOAT> > *outp)
{
#define FIX_NUMEROUS_SC_CHANNELS 1
#define MAX_IN_CHANNELS 256
    
    int numInChannels = plug->NInChannels();
    int numInScChannels = plug->NInScChannels();
    
#if FIX_NUMEROUS_SC_CHANNELS
    // Avoid a crach
    if (numInChannels > MAX_IN_CHANNELS)
        return;
#endif
    
#ifdef AAX_API
    // Protools only takes one channel for side chain
    
    // force it to 1, just in case
    if (numInScChannels > 1)
        numInScChannels = 1;
#endif
    
#if !FIX_NUMEROUS_SC_CHANNELS
    bool isInConnected[4] = { false, false, false, false };
#else
    bool isInConnected[MAX_IN_CHANNELS];
    for (int i = 0; i < MAX_IN_CHANNELS; i++)
    {
        isInConnected[i] = false;
    }
#endif
    
    for (int i = 0; i < numInChannels; i++)
    {
        isInConnected[i] = plug->IsInChannelConnected(i);
    }
    
    // in
    int numInputs = numInChannels - numInScChannels;
    
    if ((numInputs > 0) && isInConnected[0])
    {
        // FIX: Logic High Sierra crash when start playing (for plugs with sidechain)
        if (inputs[0] != NULL)
        {
            WDL_TypedBuf<BL_FLOAT> input;
            input.Resize(nFrames);
            
            if (sizeof(BL_FLOAT) == sizeof(double))
                input.Set(((BL_FLOAT **)inputs)[0], nFrames);
            else
            {
                for (int i = 0; i < nFrames; i++)
                    input.Get()[i] = inputs[0][i];
            }
            
            inp->push_back(input);
        }
    }
    
    if ((numInputs > 1) && isInConnected[1])
    {
        // FIX: Logic High Sierra crash when start playing (for plugs with sidechain)
        if (inputs[1] != NULL)
        {
            WDL_TypedBuf<BL_FLOAT> input;
            input.Resize(nFrames);
            
            if (sizeof(BL_FLOAT) == sizeof(double))
                input.Set(((BL_FLOAT **)inputs)[1], nFrames);
            else
            {
                for (int i = 0; i < nFrames; i++)
                    input.Get()[i] = inputs[1][i];
            }
            
            inp->push_back(input);
        }
    }

    // When in application mode, must deactivate sidechains
    // BUG: otherwise it crashes if we try to get sidechains
#ifndef APP_API
    // scin
#if !FIX_NUMEROUS_SC_CHANNELS
    if ((numInScChannels > 0) && isInConnected[numInChannels - numInScChannels])
    {
        // FIX: Logic High Sierra crash when start playing (for plugs with sidechain)
        if (inputs[numInChannels - numInScChannels] != NULL)
        {
            WDL_TypedBuf<BL_FLOAT> sc;
            sc.Resize(nFrames);
            
            if (sizeof(BL_FLOAT) == sizeof(double))
                sc.Set(inputs[numInChannels - numInScChannels], nFrames);
            {
                for (int i = 0; i < nFrames; i++)
                    sc.Get()[i] = inputs[numInChannels - numInScChannels][i];
            }
            
            scIn->push_back(sc);
        }
    }
    
    if ((numInScChannels > 1) && isInConnected[numInChannels - numInScChannels + 1])
    {
        // FIX: Logic High Sierra crash when start playing (for plugs with sidechain)
        if (inputs[numInChannels - numInScChannels + 1] != NULL)
        {
            WDL_TypedBuf<BL_FLOAT> sc;
            sc.Resize(nFrames);
 
            if (sizeof(BL_FLOAT) == sizeof(double))
                sc.Set(inputs[numInChannels - numInScChannels + 1], nFrames);
            else
            {
                for (int i = 0; i < nFrames; i++)
                    sc.Get()[i] = inputs[numInChannels - numInScChannels + 1][i];
            }
            
            scIn->push_back(sc);
        }
    }
#else
    for (int i = 0; i < MAX_IN_CHANNELS; i++)
    {
        if (numInChannels - numInScChannels + i >= MAX_IN_CHANNELS)
            break;
        
        if ((numInScChannels > i) &&
            isInConnected[numInChannels - numInScChannels + i])
        {
            // FIX: Logic High Sierra crash when start playing (for plugs with sidechain)
            if (inputs[numInChannels - numInScChannels + i] != NULL)
            {
                WDL_TypedBuf<BL_FLOAT> sc;
                sc.Resize(nFrames);
                
                if (sizeof(BL_FLOAT) == sizeof(double))
                    sc.Set(((BL_FLOAT **)inputs)[numInChannels -
                                                 numInScChannels + i], nFrames);
                else
                {
                    for (int j = 0; j < nFrames; j++)
                        sc.Get()[j] = inputs[numInChannels -
                                             numInScChannels + i][j];
                }
                
                scIn->push_back(sc);
            }
        }
    }
#endif
    
#endif
    
    // #bl-iplug2
#if 0
    
    // out
    if (plug->IsOutChannelConnected(0))
    {
        // FIX: Logic High Sierra crash when start playing (for plugs with sidechain)
        if (outputs[0] != NULL)
        {
            WDL_TypedBuf<BL_FLOAT> out;
            out.Resize(nFrames);
 
            //out.Set(outputs[0], nFrames);
            for (int i = 0; i < nFrames; i++)
                out.Get()[i] = outputs[0][i];
            
            outp->push_back(out);
        }
    }
    
    if (plug->IsOutChannelConnected(1))
    {
        // FIX: Logic High Sierra crash when start playing (for plugs with sidechain)
        if (outputs[1] != NULL)
        {
            WDL_TypedBuf<BL_FLOAT> out;
            out.Resize(nFrames);
            
            if (sizeof(BL_FLOAT) == sizeof(double))
                out.Set(((BL_FLOAT **)outputs)[1], nFrames);
            else
            {
                for (int i = 0; i < nFrames; i++)
                    out.Get()[i] = outputs[1][i];
            }
            
            outp->push_back(out);
        }
    }
#endif
    
#if !TRY_FIX_SIDE_CHAIN_AU
    // Set inputs and outputs to NULL if necessary
    // (will avoid later crashes)
    if (inp->size() == 1)
        inputs[1] = NULL;
    
    if (outp->size() == 1)
        outputs[1] = NULL;
#else
    // Try to make something more smart...
    
    for (int i = 0; i < numInChannels; i++)
    {
        if (inp->size() <= i)
            inputs[i] = NULL;
    }
    
    for (int i = 0; i < numInChannels; i++)
    {
        if (scIn->size() <= i)
            inputs[numInChannels + i] = NULL;
    }
   
    // #bl-iplug2
#if 0
    
    int numOutChannels = plug->NOutChannels();
    for (int i = 0; i < numOutChannels; i++)
    {
        if (outp->size() <= i)
            outputs[i] = NULL;
    }
#endif
    
#endif
}
#endif

bool
BLUtilsPlug::GetIOBuffers(int index, double *in[2], double *out[2],
                          double **inBuf, double **outBuf)
{
    *inBuf = NULL;
    *outBuf = NULL;
    
    if (out[index] != NULL)
        // We want to ouput
    {
        *outBuf = out[index];
        
        *inBuf = in[index];
        if (*inBuf == NULL)
        {
            // We have only one input
            // So take it, this is the first one
            *inBuf = in[0];
        }
        
        if (*inBuf != NULL)
            // We have both buffers
            return true;
    }
    
    // We have either no buffer, or only one out of two
    return false;
}

bool
BLUtilsPlug::GetIOBuffers(int index,
                          vector<WDL_TypedBuf<double> > &in,
                          vector<WDL_TypedBuf<double> > &out,
                          double **inBuf, double **outBuf)
{
    *inBuf = NULL;
    *outBuf = NULL;
    
    if (out.size() > index)
        // We want to ouput
    {
        *outBuf = out[index].Get();
        
        *inBuf = NULL;
        if (in.size() > index)
            *inBuf = in[index].Get();
        else
            *inBuf = in[0].Get();
        
        if (*inBuf != NULL)
            // We have both buffers
            return true;
    }
    
    // We have either no buffer, or only one out of two
    return false;
}

bool
BLUtilsPlug::GetIOBuffers(int index,
                          vector<WDL_TypedBuf<double> > &in,
                          vector<WDL_TypedBuf<double> > &out,
                          WDL_TypedBuf<double> **inBuf,
                          WDL_TypedBuf<double> **outBuf)
{
    *inBuf = NULL;
    *outBuf = NULL;
    
    if (out.size() > index)
        // We want to ouput
    {
        *outBuf = &out[index];
        
        *inBuf = NULL;
        if (in.size() > index)
            *inBuf = &in[index];
        else
            *inBuf = &in[0];
        
        if (*inBuf != NULL)
            // We have both buffers
            return true;
    }
    
    // We have either no buffer, or only one out of two
    return false;
}

bool
BLUtilsPlug::PlugIOAllZero(double *inputs[2], double *outputs[2], int nFrames)
{
    bool allZero0 = false;
    bool channelDefined0 = ((inputs[0] != NULL) && (outputs[0] != NULL));
    if (channelDefined0)
    {
        allZero0 = (BLUtils::IsAllZero(inputs[0], nFrames) &&
                    BLUtils::IsAllZero(outputs[0], nFrames));
    }
    
    bool allZero1 = false;
    bool channelDefined1 = ((inputs[1] != NULL) && (outputs[1] != NULL));
    if (channelDefined1)
    {
        allZero1 = (BLUtils::IsAllZero(inputs[1], nFrames) &&
                    BLUtils::IsAllZero(outputs[1], nFrames));
    }
    
    if (!channelDefined1 && allZero0)
        return true;
    
    if (channelDefined1 && allZero0 && allZero1)
        return true;
    
    return false;
}

bool
BLUtilsPlug::PlugIOAllZero(const vector<WDL_TypedBuf<BL_FLOAT> > &inputs,
                           const vector<WDL_TypedBuf<BL_FLOAT> > &outputs)
{
    bool allZero0 = false;
    bool channelDefined0 = ((inputs.size() > 0) && (outputs.size() > 0));
    if (channelDefined0)
    {
        allZero0 = (BLUtils::IsAllZero(inputs[0].Get(), inputs[0].GetSize()) &&
                    BLUtils::IsAllZero(outputs[0].Get(), outputs[0].GetSize()));
    }
    
    bool allZero1 = false;
    bool channelDefined1 = ((inputs.size() > 1) && (outputs.size() > 1));
    if (channelDefined1)
    {
        allZero1 = (BLUtils::IsAllZero(inputs[1].Get(), inputs[1].GetSize()) &&
                    BLUtils::IsAllZero(outputs[1].Get(), outputs[1].GetSize()));
    }
    
    if (!channelDefined1 && allZero0)
        return true;
    
    if (channelDefined1 && allZero0 && allZero1)
        return true;
    
    return false;
}

void
BLUtilsPlug::PlugCopyOutputs(const vector<WDL_TypedBuf<BL_FLOAT> > &outp,
                             double **outputs, int nFrames)
{
    for (int i = 0; i < outp.size(); i++)
    {
        if (outputs[i] == NULL)
            continue;
     
        const WDL_TypedBuf<BL_FLOAT> &out = outp[i];
        
        if (out.GetSize() == nFrames)
        {
            if (sizeof(BL_FLOAT) == sizeof(double))
                memcpy(outputs[i], out.Get(), nFrames*sizeof(double));
            else
            {
                for (int j = 0; j < nFrames; j++)
                    outputs[i][j] = out.Get()[j];
            }
        }
    }
}

int
BLUtilsPlug::PlugComputeBufferSize(int bufferSize, BL_FLOAT sampleRate)
{
    BL_FLOAT ratio = sampleRate/44100.0;
    ratio = bl_round(ratio);
    
    // FIX: Logic Auval checks for 11025 sample rate
    // So ratio would be 0.
    if (ratio < 1.0)
        ratio = 1.0;
    
    int result = bufferSize*ratio;
    
    return result;
}

// Fails sometimes...
int
BLUtilsPlug::PlugComputeLatency(Plugin *plug,
                                int nativeBufferSize, int nativeLatency,
                                BL_FLOAT sampleRate)
{
#define NATIVE_SAMPLE_RATE 44100.0
    
    // How many blocks for filling BUFFER_SIZE ?
    int blockSize = plug->GetBlockSize();
    BL_FLOAT coeff = sampleRate/NATIVE_SAMPLE_RATE;
    
    // FIX: for 48KHz and multiples
    coeff = bl_round(coeff);
    
    BL_FLOAT numBuffers = coeff*((BL_FLOAT)nativeBufferSize)/blockSize;
    if (numBuffers > (int)numBuffers)
        numBuffers = (int)numBuffers + 1;
    
    // GOOD !
    // Compute remaining, in order to compensate for
    // remaining compensation in FftProcessObj15
    BL_FLOAT remaining = numBuffers*blockSize/coeff - nativeBufferSize;
    
    BL_FLOAT newLatency = numBuffers*blockSize - (int)remaining;
    
    return newLatency;
}

// Fails sometimes...
void
BLUtilsPlug::PlugUpdateLatency(Plugin *plug,
                               int nativeBufferSize, int nativeLatency,
                               BL_FLOAT sampleRate)
{
    if (std::fabs((BL_FLOAT)(sampleRate - NATIVE_SAMPLE_RATE)) < BL_EPS)
        // We are in the native state, no need to tweek latency
    {
        plug->SetLatency(nativeLatency);
        
        return;
    }

    // Fails sometimes...
    int newLatency = BLUtilsPlug::PlugComputeLatency(plug,
                                                     nativeBufferSize, nativeLatency,
                                                     sampleRate);

    // Set latency dynamically
    // (not sure it works for all hosts)
    plug->SetLatency(newLatency);
}

BL_FLOAT
BLUtilsPlug::GetBufferSizeCoeff(Plugin *plug, int nativeBufferSize)
{
    BL_FLOAT sampleRate = plug->GetSampleRate();
    int bufferSize = BLUtilsPlug::PlugComputeBufferSize(nativeBufferSize, sampleRate);
    BL_FLOAT bufferSizeCoeff = ((BL_FLOAT)bufferSize)/nativeBufferSize;
    
    return bufferSizeCoeff;
}

bool
BLUtilsPlug::ChannelAllZero(const vector<WDL_TypedBuf<BL_FLOAT> > &channel)
{
    for (int i = 0; i < channel.size(); i++)
    {
        bool allZero = BLUtils::IsAllZero(channel[i]);
        if (!allZero)
            return false;
    }
    
    return true;
}

// Touch plug param
// When param is modified out of GUI or indirectly,
// touch the host
// => Then automations can be read
//
// NOTE: take care with Waveform Tracktion
void
BLUtilsPlug::TouchPlugParam(Plugin *plug, int paramIdx)
{
    // Force host update param, for automation
    plug->BeginInformHostOfParamChange(paramIdx);
    double normValue = plug->GetParam(paramIdx)->GetNormalized();
    //plug->GetGUI()->SetParameterFromPlug(paramIdx, normValue, true);
    plug->SetParameterValue(paramIdx, normValue);
    plug->InformHostOfParamChange(paramIdx,
                                  plug->GetParam(paramIdx)->GetNormalized());
    plug->EndInformHostOfParamChange(paramIdx);
}

void
BLUtilsPlug::SetParameterValue(Plugin *plug, int paramIdx, BL_FLOAT nonNormValue,
                               bool updateControl)
{
    if (!updateControl)
    {
        BL_FLOAT normValue = plug->GetParam(paramIdx)->ToNormalized(nonNormValue);
        plug->SetParameterValue(paramIdx, normValue);
    }
    else
    {
        // The associated control will move
        plug->SendParameterValueFromAPI(paramIdx, nonNormValue, false);
    }
}

bool
BLUtilsPlug::GetFullPlugResourcesPath(const IPluginBase &plug, WDL_String *resPath)
{
#define DUMMY_RES_FILE "dummy.txt"
    
    EResourceLocation resourceFound =
        LocateResource(DUMMY_RES_FILE,
                       "txt",
                       *resPath,
                       plug.GetBundleID(),
                       NULL, //GetWinModuleHandle(),
                       SHARED_RESOURCES_SUBPATH);// defined in plugin config.h
    
    if (resourceFound == EResourceLocation::kNotFound)
    {
        return false;
    }
    
    // Crop "/dummy.txt" from the path.
    if (resPath->GetLength() >= strlen(DUMMY_RES_FILE) + 1)
        resPath->SetLen(resPath->GetLength() - (strlen(DUMMY_RES_FILE) + 1));
    
    return true;
}

void
BLUtilsPlug::ApplyGain(const vector<WDL_TypedBuf<BL_FLOAT> > &in,
                       vector<WDL_TypedBuf<BL_FLOAT> > *out,
                       ParamSmoother2 *smoother)
{
    if (in.empty())
        return;

    if (smoother == NULL)
        return;
    
    for (int i = 0; i < in[0].GetSize(); i++)
    {
        BL_FLOAT gain = smoother->Process();

        BL_FLOAT ls = in[0].Get()[i];
        (*out)[0].Get()[i] = gain*ls;
    
        if ((in.size() > 1) && (out->size() > 1))
        {
            BL_FLOAT rs = in[1].Get()[i];
            (*out)[1].Get()[i] = gain*rs;
        }
    }
}

// Find fps in global config file if any
int
BLUtilsPlug::GetPlugFPS(int defaultFPS)
{
    // The file is in th form:
    //
    // [BlueLab]
    // fps=30

        
#ifdef __linux__
    char fileName[FILENAME_SIZE];
    sprintf(fileName, "%s/.config/BlueLab.ini", getenv("HOME"));
        
    FILE *file = fopen(fileName, "r");
    if (file == NULL)
        return defaultFPS;
    
    fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, 0, SEEK_SET);
	char *data = (char*)malloc(size + 1);
	fread(data, 1, size, file);
	data[size] = '\0';
    
    fclose(file);

    ini_t *ini = ini_load(data, NULL);
	free(data);
    
    int section = ini_find_section(ini, "BlueLab", 0);
    int fps_index = ini_find_property(ini, section, "fps", strlen("fps"));
	char const *fpsStr = ini_property_value(ini, section, fps_index);
    
    if (fpsStr == NULL)
    {
        ini_destroy(ini);

        return defaultFPS;
    }
    
    int fps = atoi(fpsStr);

    if ((fps > 0) && (fps <= 120))
    {
        ini_destroy(ini);
        
        return fps;
    }
    
    ini_destroy(ini);
#endif

    return defaultFPS;
}

BL_FLOAT
BLUtilsPlug::GetTransportTime(Plugin *plug)
{
    BL_FLOAT sampleRate = plug->GetSampleRate();
    BL_FLOAT samplePos = plug->GetTransportSamplePos();
    if (samplePos < 0.0)
        return 0.0;
    
    BL_FLOAT transportTime = samplePos/sampleRate;

    return transportTime;
}
