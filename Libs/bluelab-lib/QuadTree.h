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
//  QuadTree.h
//  BL-StereoViz
//
//  Created by applematuer on 11/17/18.
//
//

#ifndef __BL_StereoViz__QuadTree__
#define __BL_StereoViz__QuadTree__

#ifdef IGRAPHICS_NANOVG

#include <vector>
using namespace std;

#include <SparseVolRender.h>

//

class QuadTree
{
public:
    class PointMerger
    {
    public:
        PointMerger();
        
        virtual ~PointMerger();
        
        virtual bool IsMergeable(const SparseVolRender::Point &p0, const SparseVolRender::Point &p1) const = 0;
        
        bool IsMergeable(const vector<SparseVolRender::Point> &points) const;
        
        virtual void Merge(const vector<SparseVolRender::Point> &points, SparseVolRender::Point *res) const = 0;
    };
    
    //
    QuadTree(BL_FLOAT bbox[2][2], int level);
    
    virtual ~QuadTree();
    
    static QuadTree *BuildFromBottom(BL_FLOAT bbox[2][2], int resolution[2]);
    
    void Clear();
    
    static void InsertPoints(QuadTree *root, const vector<SparseVolRender::Point> &points);
    
    static void MergePoints(QuadTree *root, const PointMerger &merger);
    
    static void GetPoints(QuadTree *root, vector<SparseVolRender::Point> *points);
    
protected:
    void InsertPoint(const SparseVolRender::Point &points);
    
    void GetPoints(vector<SparseVolRender::Point> *points);
    
    void MergePoints(const PointMerger &merger, bool *merged);
    
    // UNUSED
    void GetBottomNodes(vector<QuadTree *> *bottomNodes);
    
    bool IsLeaf();
    
    
    QuadTree *mChildren[4];

    BL_FLOAT mBBox[2][2];
    
    vector<SparseVolRender::Point> mPoints;
    
    int mLevel;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_StereoViz__QuadTree__) */
