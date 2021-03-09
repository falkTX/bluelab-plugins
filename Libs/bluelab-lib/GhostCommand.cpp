#include "GhostCommand.h"

GhostCommand::GhostCommand(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;

    mScale = new Scale();
}

GhostCommand::~GhostCommand()
{
    delete mScale;
}

void
GhostCommand::SetSelection(BL_FLOAT x0, BL_FLOAT y0, BL_FLOAT x1, BL_FLOAT y1,
                           Scale::Type yScale)
{    
    y0 = mScale->ApplyScaleInv(yScale, y0,
                               (BL_FLOAT)0.0, (BL_FLOAT)(mSampleRate*0.5));
    y1 = mScale->ApplyScaleInv(yScale, y1,
                               (BL_FLOAT)0.0, (BL_FLOAT)(mSampleRate*0.5));
    
    
    if (y0 < 0.0)
        y0 = 0.0;
    
    if (y1 > 1.0)
        y1 = 1.0;
    
    mSelection[0] = x0;
    mSelection[1] = y0;
    mSelection[2] = x1;
    mSelection[3] = y1;
}

void
GhostCommand::ApplySlice(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                         vector<WDL_TypedBuf<BL_FLOAT> > *phases)
{
    DataToBuf(*magns, &mSavedMagnsSlice);
    DataToBuf(*phases, &mSavedPhasesSlice);
    
    vector<WDL_TypedBuf<BL_FLOAT> > magnsSel;
    ExtractAux(&magnsSel, *magns);
    
    vector<WDL_TypedBuf<BL_FLOAT> > phasesSel;
    ExtractAux(&phasesSel, *phases);
    
    Apply(&magnsSel, &phasesSel);
    
    // Magns
    for (int i = 0; i < magnsSel.size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &magns0 = magnsSel[i];
        
        (*magns)[i] = magns0;
    }
    
    // Phases
    for (int i = 0; i < phasesSel.size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &phases0 = phasesSel[i];
        
        (*phases)[i] = phases0;
    }
}

void
GhostCommand::GetSelection(BL_FLOAT *x0, BL_FLOAT *y0, BL_FLOAT *x1, BL_FLOAT *y1,
                           Scale::Type yScale) const
{
    *x0 = mSelection[0];
    *y0 = mSelection[1];
    *x1 = mSelection[2];
    *y1 = mSelection[3];
    
    *y0 = mScale->ApplyScale(yScale, *y0,
                             (BL_FLOAT)0.0, (BL_FLOAT)(mSampleRate*0.5));
    *y1 = mScale->ApplyScale(yScale, *y1,
                             (BL_FLOAT)0.0, (BL_FLOAT)(mSampleRate*0.5));
}

void
GhostCommand::Undo(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                   vector<WDL_TypedBuf<BL_FLOAT> > *phases)
{    
    BufToData(magns, mSavedMagnsSlice);
    BufToData(phases, mSavedPhasesSlice);
}

void
GhostCommand::GetSelectedData(const vector<WDL_TypedBuf<BL_FLOAT> > &data,
                              WDL_TypedBuf<BL_FLOAT> *selectedData)
{
    int x0;
    int x1;
    int y0;
    int y1;
    bool res = GetDataBounds(data, &x0, &y0, &x1, &y1);
    if (!res)
        return;
    
    int w = x1 - x0;
    int h = y1 - y0;
    
    selectedData->Resize(w*h);
    
    for (int i = 0; i < w; i++)
    {
        if (i + x0 >= data.size())
            continue;
            
        const WDL_TypedBuf<BL_FLOAT> &col = data[i + x0];
        for (int j = 0; j < h; j++)
        {
            if (j + y0 >= col.GetSize())
                continue;
            
            BL_FLOAT val = col.Get()[j + y0];
            
            selectedData->Get()[i + j*w] = val;
        }
    }
}

// A slice have already been extracted
void
GhostCommand::GetSelectedDataY(const vector<WDL_TypedBuf<BL_FLOAT> > &data,
                               WDL_TypedBuf<BL_FLOAT> *selectedData)
{
    int y0;
    int y1;
    bool res = GetDataBoundsSlice(data, &y0, &y1);
    if (!res)
        return;
    
    int h = y1 - y0;
    int w = (int)data.size();
    
    selectedData->Resize(w*h);
    
    for (int i = 0; i < w; i++)
    {
        const WDL_TypedBuf<BL_FLOAT> &col = data[i];
        for (int j = 0; j < h; j++)
        {
            if (j + y0 >= col.GetSize())
                continue;
            
            BL_FLOAT val = col.Get()[j + y0];
            
            selectedData->Get()[i + j*w] = val;
        }
    }
}

void
GhostCommand::ReplaceSelectedData(vector<WDL_TypedBuf<BL_FLOAT> > *data,
                                  const WDL_TypedBuf<BL_FLOAT> &selectedData)
{
    // Save original data (for history mechanisme)
    GetSelectedData(*data, &mSavedData);
    
    int x0;
    int x1;
    int y0;
    int y1;
    bool res = GetDataBounds(*data, &x0, &y0, &x1, &y1);
    if (!res)
        return;
    
    int width = x1 - x0;
    int height = y1 - y0;
    
    // Test just in case
    if (selectedData.GetSize() != width*height)
        return;
    
    for (int i = 0; i < width; i++)
    {
        // Prevent pasting outside the highest bound of the data for example
        if (i + x0 >= data->size())
            continue;
        
        WDL_TypedBuf<BL_FLOAT> &col = (*data)[i + x0];
        for (int j = 0; j < height; j++)
        {
            if (i + j*width >= selectedData.GetSize())
                continue;
            
            BL_FLOAT val = selectedData.Get()[i + j*width];
            
            if (j + y0 >= col.GetSize())
                continue;
            
            col.Get()[j + y0] = val;
        }
    }
}

void
GhostCommand::ReplaceSelectedDataY(vector<WDL_TypedBuf<BL_FLOAT> > *data,
                                   const WDL_TypedBuf<BL_FLOAT> &selectedData)
{
    // Save original data (for history mechanisme)
    GetSelectedDataY(*data, &mSavedData);
    
    int y0;
    int y1;
    bool res = GetDataBoundsSlice(*data, &y0, &y1);
    if (!res)
        return;
    
    int width = data->size();
    int height = y1 - y0;
    
    // Test just in case
    if (selectedData.GetSize() != width*height)
        return;
    
    for (int i = 0; i < width; i++)
    {
        WDL_TypedBuf<BL_FLOAT> &col = (*data)[i];
        for (int j = 0; j < height; j++)
        {
            if (i + j*width >= selectedData.GetSize())
                continue;
            
            BL_FLOAT val = selectedData.Get()[i + j*width];
            
            if (j + y0 >= col.GetSize())
                continue;
            
            col.Get()[j + y0] = val;
        }
    }
}

void
GhostCommand::DataToBuf(const vector<WDL_TypedBuf<BL_FLOAT> > &data,
                        WDL_TypedBuf<BL_FLOAT> *buf)
{
    int w = data.size();
    if (w == 0)
        return;
    
    int h = data[0].GetSize();;
    
    buf->Resize(w*h);
    
    for (int i = 0; i < w; i++)
    {
        const WDL_TypedBuf<BL_FLOAT> &col = data[i];
        for (int j = 0; j < h; j++)
        {
            BL_FLOAT val = col.Get()[j];
            
            buf->Get()[i + j*w] = val;
        }
    }
}

void
GhostCommand::BufToData(vector<WDL_TypedBuf<BL_FLOAT> > *data,
                        const WDL_TypedBuf<BL_FLOAT> &buf)
{
    int w = data->size();
    if (w == 0)
        return;
    
    int h = (*data)[0].GetSize();
    
    if (buf.GetSize() != w*h)
        return;
    
    for (int i = 0; i < w; i++)
    {
        WDL_TypedBuf<BL_FLOAT> &col = (*data)[i];
        for (int j = 0; j < h; j++)
        {
            BL_FLOAT val = buf.Get()[i + j*w];
            
            col.Get()[j] = val;
        }
    }
}


bool
GhostCommand::GetDataBounds(const vector<WDL_TypedBuf<BL_FLOAT> > &data,
                            int *x0, int *y0, int *x1, int *y1)
{
    if (data.empty())
        return false;
    if (data[0].GetSize() == 0)
        return false;
    
    // Make special trick to keep the width and height,
    // avoiding rounding errors
    // And that works !
    //
    BL_FLOAT width = mSelection[2] - mSelection[0];
    BL_FLOAT height = mSelection[3] - mSelection[1];
    
    BL_FLOAT x0f = mSelection[0]*data.size();
    *x0 = round(x0f);
    
    BL_FLOAT y0f = mSelection[1]*data[0].GetSize();
    *y0 = round(y0f);
    
    BL_FLOAT x1f = *x0 + width*data.size();
    *x1 = round(x1f);
    
    BL_FLOAT y1f = *y0 + height*data[0].GetSize();
    *y1 = round(y1f);
    
    return true;
}

bool
GhostCommand::GetDataBoundsSlice(const vector<WDL_TypedBuf<BL_FLOAT> > &data,
                                 int *y0, int *y1)
{
    if (data.empty())
        return false;
    if (data[0].GetSize() == 0)
        return false;
    
    BL_FLOAT height = mSelection[3] - mSelection[1];
    
    BL_FLOAT y0f = mSelection[1]*data[0].GetSize();
    *y0 = round(y0f);
    
    BL_FLOAT y1f = *y0 + height*data[0].GetSize();
    *y1 = round(y1f);
    
    return true;
}


void
GhostCommand::ExtractAux(vector<WDL_TypedBuf<BL_FLOAT> > *dataSel,
                         const vector<WDL_TypedBuf<BL_FLOAT> > &data)
{
    for (int i = 0; i < (int)data.size(); i++)
    {
        const WDL_TypedBuf<BL_FLOAT> &data0 = data[i];
    
        dataSel->push_back(data0);
    }
}
