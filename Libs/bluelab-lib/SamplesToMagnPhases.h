#ifndef SAMPLES_TO_MAGN_PHASES_H
#define SAMPLES_TO_MAGN_PHASES_H

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class FftProcessObj16;
class SpectroEditFftObj3;
class SamplesPyramid2;

// Very accurate conversion, sample aligned
// Used for Ghost edition
class SamplesToMagnPhases
{
 public:
    // Samples is the different samples we are working on
    // It can correspond to a whole file
    SamplesToMagnPhases(vector<WDL_TypedBuf<BL_FLOAT> > *samples,
                        FftProcessObj16 *fftObj,
                        SpectroEditFftObj3 *spectroEditObjs[2],
                        SamplesPyramid2 *samplesPyramid = NULL);
    virtual ~SamplesToMagnPhases();
    
    void ReadSpectroDataSlice(vector<WDL_TypedBuf<BL_FLOAT> > magns[2],
                              vector<WDL_TypedBuf<BL_FLOAT> > phases[2],
                              BL_FLOAT minXNorm, BL_FLOAT maxNormX);

    void WriteSpectroDataSlice(vector<WDL_TypedBuf<BL_FLOAT> > magns[2],
                               vector<WDL_TypedBuf<BL_FLOAT> > phases[2],
                               BL_FLOAT minXNorm, BL_FLOAT maxNormX,
                               int fadeNumSamples = 0);

 protected:
    struct ReadWriteSliceState
    {
        SpectroEditFftObj3::Mode mSpectroEditMode;
        bool mSpectroEditSelectionEnabled;
        BL_FLOAT mSpectroEditSels[2][4];
    };
    void ReadWriteSliceSaveState(ReadWriteSliceState *state);
    void ReadWriteSliceRestoreState(const ReadWriteSliceState &state);

    void AlignSamplePosToFftBuffers(BL_FLOAT *pos);

    // Set spectro edit bounds, in samples
    void SetDataSelection(BL_FLOAT minXSamples, BL_FLOAT maxXSamples);

    // Normalized pos to samples pos
    // well adjusted for further processing
    void NormalizedPosToSamplesPos(BL_FLOAT minXNorm,
                                   BL_FLOAT maxXNorm,
                                   BL_FLOAT *minXSamples,
                                   BL_FLOAT *maxXSamples);
    
    vector<WDL_TypedBuf<BL_FLOAT> > *mSamples;
    
    FftProcessObj16 *mFftObj;
    SpectroEditFftObj3 *mSpectroEditObjs[2];

    SamplesPyramid2 *mSamplesPyramid;

 private:
    // Tmp buffers
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf0;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf1;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf2;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf3;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf4;
};

#endif
