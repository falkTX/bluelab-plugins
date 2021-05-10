// Modified version of SMB's pitch shift to use the algorithm described in
// Nicolas Juillerat & Beat Hirsbrunner's 2010 paper "LOW LATENCY AUDIO PITCH
// SHIFTING IN THE FREQUENCY DOMAIN".

#include <string.h>
#include <math.h>
#include <stdio.h>

// #bluelab
//#include <Accelerate/Accelerate.h>

#define MAX_FRAME_LENGTH 8192

#define TWOPI 6.2831853071795864

// #bluelab
extern void smbFft(float *fftBuffer, long fftFrameSize, long sign);
    
#if 0 // #bluelab
inline void accelFFT(FFTSetup &setup, float *real, float *imag, long fftFrameSize, FFTDirection direction) {
  DSPSplitComplex complex = {real, imag};
  vDSP_fft_zip(setup, &complex, 1, log2(fftFrameSize), direction);
}
#endif
    
struct JHState {
  float inFIFO[MAX_FRAME_LENGTH];
  float outFIFO[MAX_FRAME_LENGTH];
  float fftReal[MAX_FRAME_LENGTH];
  float fftImag[MAX_FRAME_LENGTH];
  float scaledReal[MAX_FRAME_LENGTH];
  float scaledImag[MAX_FRAME_LENGTH];
  float outputAccum[2*MAX_FRAME_LENGTH];
  long rover = 0;
  unsigned long frameNum = 0;

  // #bluelab
  //FFTSetup setup;
  float smbFfftBuffer[2*MAX_FRAME_LENGTH];
};

extern "C" {

void *initJHState() {
  JHState *state = new JHState();

  memset(state->inFIFO, 0, MAX_FRAME_LENGTH*sizeof(float));
  memset(state->outFIFO, 0, MAX_FRAME_LENGTH*sizeof(float));
  memset(state->fftReal, 0, MAX_FRAME_LENGTH*sizeof(float));
  memset(state->fftImag, 0, MAX_FRAME_LENGTH*sizeof(float));
  memset(state->outputAccum, 0, 2*MAX_FRAME_LENGTH*sizeof(float));

  // #bluelab
  //state->setup = vDSP_create_fftsetup(log2(MAX_FRAME_LENGTH), FFT_RADIX2);

  return state;
}

void destroyJHState(void *state) {
  JHState *smbState = (JHState *)state;
  delete smbState;
}

void jhPitchShift(void *stateVoid, float pitchShift, long numSampsToProcess, long fftFrameSize, long osamp, long synthFactor, float sampleRate, const float *indata, float *outdata)
{
  double window;
  long i, k;

  JHState *state = (JHState *)stateVoid;

  /* set up some handy variables */
  long fftFrameSize2 = fftFrameSize/2;
  long stepSize = fftFrameSize/osamp;
  long inFifoLatency = fftFrameSize-stepSize;
  long outFrameFullSize = fftFrameSize * synthFactor;
  if (state->rover == 0) state->rover = inFifoLatency;

  /* main processing loop */
  for (i = 0; i < numSampsToProcess; i++){

    /* As long as we have not yet collected enough data just read in */
    state->inFIFO[state->rover] = indata[i];
    outdata[i] = state->outFIFO[state->rover-inFifoLatency];
    state->rover++;

    /* now we have enough data for processing */
    if (state->rover >= fftFrameSize) {
      state->rover = inFifoLatency;

      /* do windowing */
      memset(state->fftImag, 0, MAX_FRAME_LENGTH*sizeof(float));
      for (k = 0; k < fftFrameSize; k++) {
        window = -.5*cos(TWOPI*(double)k/(double)fftFrameSize)+.5;
        state->fftReal[k] = state->inFIFO[k] * window;
      }

      /* ***************** ANALYSIS / PROCESSING ******************* */
      /* do transform */
      // #bluelav
      //accelFFT(state->setup, state->fftReal, state->fftImag, fftFrameSize, kFFTDirection_Forward);
      memset(state->smbFfftBuffer, 0, 2*MAX_FRAME_LENGTH*sizeof(float));
      for (int k = 0; k < fftFrameSize; k++)
      {
          state->smbFfftBuffer[k*2] = state->fftReal[k];
      }
      smbFft(state->smbFfftBuffer, fftFrameSize, -1);
      for (int k = 0; k < fftFrameSize; k++)
      {
          state->fftReal[k] = state->smbFfftBuffer[k*2];
          state->fftImag[k] = state->smbFfftBuffer[k*2 + 1];
      }
      //
      
      memset(state->scaledReal, 0, MAX_FRAME_LENGTH*sizeof(float));
      memset(state->scaledImag, 0, MAX_FRAME_LENGTH*sizeof(float));

      float multiplier = -(TWOPI * (float)(state->frameNum)) / (float)(osamp * synthFactor * fftFrameSize);

      /* Scale each phasor to its new bin and correct the phase */
      for (int bin = 0; bin <= fftFrameSize2; bin++) {
        // b := ⌊mka + 0.5⌋
        int newBin = (int)(((float)(synthFactor * bin) * pitchShift) + 0.5);
        if (newBin >= 0 && newBin < outFrameFullSize) {
          float real = state->fftReal[bin];
          float imag = state->fftImag[bin];
          // expanded form of `Ω := Ωe^(−i(b−ma)2πp/OmN)` where multiplier = `-2πp/OmN`
          float oscVar = (float)(newBin - (synthFactor * bin)) * multiplier;
          state->scaledReal[newBin] = (real * cos(oscVar)) - (imag * sin(oscVar));
          state->scaledImag[newBin] = (real * sin(oscVar)) + (imag * cos(oscVar));
        }
      }

      /* ***************** SYNTHESIS ******************* */
      /* do inverse transform */
      // #bluelab
      //accelFFT(state->setup, state->scaledReal, state->scaledImag, outFrameFullSize, kFFTDirection_Inverse);
      for (int k = 0; k < fftFrameSize; k++)
      {
          state->smbFfftBuffer[k*2] = state->scaledReal[k];
          state->smbFfftBuffer[k*2 + 1] = state->scaledImag[k];
      }
      smbFft(state->smbFfftBuffer, fftFrameSize, 1);
      for (int k = 0; k < fftFrameSize; k++)
      {
          state->scaledReal[k] = state->smbFfftBuffer[k*2];
      }
      
      //
      
      /* do windowing and add to output accumulator */ 
      for(k=0; k < fftFrameSize; k++) {
        window = -.5*cos(TWOPI*(double)k/(double)fftFrameSize)+.5;
        state->outputAccum[k] += 2.*window*state->scaledReal[k] / (fftFrameSize2*osamp);
      }
      memcpy(state->outFIFO, state->outputAccum, stepSize * sizeof(float));

      /* shift accumulator */
      memmove(state->outputAccum, state->outputAccum+stepSize, fftFrameSize*sizeof(float));

      /* move input FIFO */
      memcpy(state->inFIFO, state->inFIFO + stepSize, inFifoLatency * sizeof(float));

      state->frameNum++;
      if (state->frameNum == (osamp * synthFactor)) {
        state->frameNum = 0;
      }
    }
  }
}

}
