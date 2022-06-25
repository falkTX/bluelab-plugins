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
 
//
//  GraphCurve.h
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#ifndef GraphAxis2_h
#define GraphAxis2_h

#include <string>
using namespace std;

#include <BLTypes.h>
#include <GraphCurve5.h>
#include <Scale.h>

#include "IPlug_include_in_plug_hdr.h"

class GraphControl12;
class GraphAxis2
{
public:
    enum ViewOrientation
    {
        HORIZONTAL = 0,
        VERTICAL
    };
    
    GraphAxis2();
    
    virtual ~GraphAxis2();
    
    void InitHAxis(Scale::Type scale,
                   BL_GUI_FLOAT minX, BL_GUI_FLOAT maxX,
                   int axisColor[4], int axisLabelColor[4],
                   BL_GUI_FLOAT lineWidth,
                   BL_GUI_FLOAT offsetY = 0.0,
                   int axisOverlayColor[4] = NULL,
                   BL_GUI_FLOAT fontSizeCoeff = 1.0,
                   int axisLinesOverlayColor[4] = NULL);
    
    void InitVAxis(Scale::Type scale,
                   BL_GUI_FLOAT minY, BL_GUI_FLOAT maxY,
                   int axisColor[4], int axisLabelColor[4],
                   BL_GUI_FLOAT lineWidth,
                   //BL_GUI_FLOAT offset = 0.0, BL_GUI_FLOAT offsetX = 0.0,
                   BL_GUI_FLOAT offsetX = 0.0, BL_GUI_FLOAT offsetY = 0.0,
                   int axisOverlayColor[4] = NULL,
                   BL_GUI_FLOAT fontSizeCoeff = 1.0,
                   bool alignTextRight = false,
                   int axisLinesOverlayColor[4] = NULL,
                   bool alignRight = true);
    
    void SetMinMaxValues(BL_GUI_FLOAT minVal, BL_GUI_FLOAT maxVal);
    
    void SetData(char *data[][2], int numData);
    
    // Screen bounds [0, 1] by default
    void SetBounds(BL_FLOAT bounds[2]);

    void SetAlignToScreenPixels(bool flag);

    void SetOffsetPixels(BL_FLOAT offsetPixels);

    // Align the first and last labels to the borders of the graph?
    void SetAlignBorderLabels(bool flag);

    // View orientation (for Panogram)
    void SetViewOrientation(ViewOrientation orientation);
    
    // Set to -1 to not force (and keep th default)
    void SetForceLabelHAlign(int align);

    void SetScaleType(Scale::Type scaleType);
    
protected:
    friend class GraphControl12;
    void SetGraph(GraphControl12 *graph);
    
    void NotifyGraph();
    
    void InitAxis(int axisColor[4],
                  int axisLabelColor[4],
                  int axisLabelOverlayColor[4],
                  int axisLinesOverlayColor[4],
                  BL_GUI_FLOAT lineWidth);
    
    typedef struct
    {
        BL_GUI_FLOAT mT;
        string mText;
    } GraphAxisData;

    friend class GraphControl12;
    
    //
    vector<GraphAxisData> mValues;
    
    Scale::Type mScaleType;
    BL_FLOAT mMinVal;
    BL_FLOAT mMaxVal;
    
    int mColor[4];
    int mLabelColor[4];
    
    // Hack
    //BL_GUI_FLOAT mOffset;
    
    // To be able to display the axis on the right
    BL_GUI_FLOAT mOffsetX;
    BL_GUI_FLOAT mOffsetY;
    
    // Overlay axis labels ?
    bool mOverlay;
    int mLabelOverlayColor[4];
    
    // Overlay axis lines ?
    bool mLinesOverlay;
    int mLinesOverlayColor[4];
    
    BL_GUI_FLOAT mFontSizeCoeff;
    
    bool mAlignTextRight;
    bool mAlignRight;
    
    BL_GUI_FLOAT mLineWidth;
    
    BL_GUI_FLOAT mBounds[2];
    
    GraphControl12 *mGraph;

    Scale *mScale;

    bool mAlignToScreenPixels;

    // For time axis scroll
    BL_FLOAT mOffsetPixels;

    bool mAlignBorderLabels;

    ViewOrientation mViewOrientation;
    // Hack, for aligning after Panogram view rotation
    // Set to -1 to ignore
    int mForceLabelHAlign;
};

#endif
