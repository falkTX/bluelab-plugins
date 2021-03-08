#ifndef GHOST_COMMAND_H
#define GHOST_COMMAND_H

#include <vector>
using namespace std;

#include <BLTypes.h>

#include <Scale.h>

#include "IPlug_include_in_plug_hdr.h"

class Ghost;
class GhostCommand
{
public:
    GhostCommand(BL_FLOAT sampleRate);
    
    virtual ~GhostCommand();
    
    // Coordinates are normalized coordinates on data
    void SetSelection(BL_FLOAT x0, BL_FLOAT y0, BL_FLOAT x1, BL_FLOAT y1,
                      Scale::Type yScale);
    
    void GetSelection(BL_FLOAT *x0, BL_FLOAT *y0, BL_FLOAT *x1, BL_FLOAT *y1,
                      Scale::Type yScale) const;
    
    // Provide an "overflow", since the extracted data slice has extra magins in x,
    // for good reconstruction
    virtual void ApplySlice(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                            vector<WDL_TypedBuf<BL_FLOAT> > *phases,
                            int keepBorderSize);
    
    virtual void Apply(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                       vector<WDL_TypedBuf<BL_FLOAT> > *phases) = 0;
    
    virtual void Undo(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                      vector<WDL_TypedBuf<BL_FLOAT> > *phases);
    
protected:
    friend class Ghost;
    
    // This is the full data
    void GetSelectedData(const vector<WDL_TypedBuf<BL_FLOAT> > &data,
                         WDL_TypedBuf<BL_FLOAT> *selectedData);

    // A slice have already been extracted from the full data
    void GetSelectedDataY(const vector<WDL_TypedBuf<BL_FLOAT> > &data,
                          WDL_TypedBuf<BL_FLOAT> *selectedData);
    
    void ReplaceSelectedData(vector<WDL_TypedBuf<BL_FLOAT> > *data,
                             const WDL_TypedBuf<BL_FLOAT> &selectedData);
    
    void ReplaceSelectedDataY(vector<WDL_TypedBuf<BL_FLOAT> > *data,
                              const WDL_TypedBuf<BL_FLOAT> &selectedData);
    
    bool GetDataBounds(const vector<WDL_TypedBuf<BL_FLOAT> > &data,
                       int *startX, int *startY,
                       int *endX, int *endY);

    // Get only y bounds, since x bounds correcpond to the full data
    // (the slice has already been extracted between x0 and x1)
    bool GetDataBoundsSlice(const vector<WDL_TypedBuf<BL_FLOAT> > &data,
                            int *y0, int *y1);

    void ExtractAux(vector<WDL_TypedBuf<BL_FLOAT> > *dataSel,
                    const vector<WDL_TypedBuf<BL_FLOAT> > &data,
                    int keepBorderSize);

    void DataToBuf(const vector<WDL_TypedBuf<BL_FLOAT> > &data,
                   WDL_TypedBuf<BL_FLOAT> *buf);
    
    void BufToData(vector<WDL_TypedBuf<BL_FLOAT> > *data,
                   const WDL_TypedBuf<BL_FLOAT> &buf);
    
    // Assume the selection is not changing for a given command
    BL_FLOAT mSelection[4];
    
    // For undo mechanism
    WDL_TypedBuf<BL_FLOAT> mSavedData;
    
    // Same, for with slice extraction mechanism
    WDL_TypedBuf<BL_FLOAT> mSavedMagnsSlice;
    WDL_TypedBuf<BL_FLOAT> mSavedPhasesSlice;
    
    BL_FLOAT mSampleRate;

    Scale *mScale;
};

#endif
