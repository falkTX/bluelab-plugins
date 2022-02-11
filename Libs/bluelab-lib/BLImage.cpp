#include <ColorMap4.h>
#include <BLImage.h>
#include <BLUtils.h>

BLImage::BLImage(int width ,int height, bool useGLSL)
{
  mWidth = width;
  mHeight = height;

  mData.Resize(mWidth*mHeight);
  BLUtils::FillAllZero(&mData);

  mRange = 0.0;
  mContrast = 0.5;

  mAlpha = 1.0;

  mColorMap = NULL;
  mColorMapFactory = new ColorMapFactory(useGLSL, false);
  
  SetColorMap(ColorMapFactory::COLORMAP_BLUE);

  mDataChanged = true;
  mColormapDataChanged = true;
}

BLImage::~BLImage()
{
  if (mColorMap != NULL)
    delete mColorMap;
  
  if (mColorMapFactory != NULL)
    delete mColorMapFactory;
}

void
BLImage::Reset()
{
  BLUtils::FillAllZero(&mData);
  
  mDataChanged = true;
  mColormapDataChanged = true;
}

int
BLImage::GetWidth()
{
  return mWidth;
}

int
BLImage::GetHeight()
{
  return mHeight;
} 

void
BLImage::GetData(WDL_TypedBuf<BL_FLOAT> *data)
{
  *data = mData;
}

void
BLImage::SetData(const WDL_TypedBuf<BL_FLOAT> &data)
{
  mData = data;

  mDataChanged = true;
}

void
BLImage::SetData(int width, int height, const WDL_TypedBuf<BL_FLOAT> &data)
{
    mWidth = width;
    mHeight = height;
    
    mData = data;
    
    mDataChanged = true;
}

void
BLImage::TouchData()
{
  mDataChanged = true;
}

// Optimized version: keep the strict minimum
bool
BLImage::GetImageDataFloat(WDL_TypedBuf<unsigned char> *buf)
{
  if (!mDataChanged)
    return false;

  buf->Resize(mWidth*mHeight*4);
  unsigned char *buf0 = buf->Get();
  
  // Empty the buffer
  // Because the spectrogram may be not totally full
  memset(buf0, 0, mWidth*mHeight*4);
  
  // Data
  BL_FLOAT *buf1 = mData.Get();
  for (int j = 0; j < mHeight; j++)
  {
    for (int i = 0; i < mWidth; i++)
    {
      BL_FLOAT value = buf1[i + j*mWidth];
      if (value > 1.0)
          value = 1.0;
      
      int pixIdx = (mHeight - 1 - j)*mWidth + i;
      ((float *)buf0)[pixIdx] = (float)value;
    }
  }
  
  mDataChanged = false;
  
  return true;
}

bool
BLImage::GetColormapImageDataRGBA(WDL_TypedBuf<unsigned int> *colormapImageData)
{
    if (!mColormapDataChanged)
        return false;
    
    mColorMap->GetDataRGBA(colormapImageData);
    
    mColormapDataChanged = false;
    
    return true;
}

void
BLImage::SetRange(BL_FLOAT range)
{
  mRange = range;
  
  mColorMap->SetRange(mRange);
  mColorMap->Generate();
  
  mColormapDataChanged = true;
}

void
BLImage::SetContrast(BL_FLOAT contrast)
{
  mContrast = contrast;
  
  mColorMap->SetContrast(mContrast);
  mColorMap->Generate();
  
  mColormapDataChanged = true;
}

void
BLImage::SetColorMap(ColorMapFactory::ColorMap colorMapId)
{
  if (mColorMap != NULL)
    delete mColorMap;
    
  mColorMap = mColorMapFactory->CreateColorMap(colorMapId);
  
  // Forward the current parameters
  mColorMap->SetRange(mRange);
  mColorMap->SetContrast(mContrast);
  
  mColormapDataChanged = true;
}

void
BLImage::TouchColorMap()
{
  mColormapDataChanged = true;
}

BL_FLOAT
BLImage::GetAlpha()
{
    return mAlpha;
}

void
BLImage::SetAlpha(BL_FLOAT alpha)
{
  mAlpha = alpha;
}
