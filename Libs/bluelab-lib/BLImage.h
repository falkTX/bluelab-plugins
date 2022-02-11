#ifndef BL_IMAGE_H
#define BL_IMAGE_H

#include <BLTypes.h>
#include <ColorMapFactory.h>

#include "IPlug_include_in_plug_hdr.h"

class ColorMap4;
class BLImage
{
 public:
  BLImage(int width, int height, bool useGLSL = true);

  virtual ~BLImage();

  void Reset();
    
  int GetWidth();
  int GetHeight();

  void GetData(WDL_TypedBuf<BL_FLOAT> *data);
  void SetData(const WDL_TypedBuf<BL_FLOAT> &data);
  void SetData(int width, int height, const WDL_TypedBuf<BL_FLOAT> &data);
  void TouchData();
  
  void SetRange(BL_FLOAT range);
  void SetContrast(BL_FLOAT contrast);

  void SetColorMap(ColorMapFactory::ColorMap colorMapId);
  void TouchColorMap();

  BL_FLOAT GetAlpha();
  void SetAlpha(BL_FLOAT alpha);

  bool GetImageDataFloat(WDL_TypedBuf<unsigned char> *buf);
  bool GetColormapImageDataRGBA(WDL_TypedBuf<unsigned int> *colormapImageData);
  
 protected:
  int mWidth;
  int mHeight;

  WDL_TypedBuf<BL_FLOAT> mData;

  BL_FLOAT mRange;
  BL_FLOAT mContrast;
    
  ColorMapFactory *mColorMapFactory;
  ColorMap4 *mColorMap;

  BL_FLOAT mAlpha;
  
  bool mDataChanged;
  bool mColormapDataChanged;
};
#endif
