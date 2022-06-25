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
//  RCQuadTree.h
//  BL-StereoViz
//
//  Created by applematuer on 11/17/18.
//
//

#ifndef __BL_StereoViz__RCQuadTree__
#define __BL_StereoViz__RCQuadTree__

#ifdef IGRAPHICS_NANOVG

#include <vector>
using namespace std;

#include <RayCaster2.h>


// Quad tree for RayCaster(1 or 2)

class RCQuadTree
{
public:
    class Visitor
    {
    public:
        Visitor();
        
        virtual ~Visitor();
        
        virtual bool Visit(RCQuadTree *node) const = 0;
    };
    
    //
    RCQuadTree(RC_FLOAT bbox[2][2], int level);
    
    virtual ~RCQuadTree();
    
    static RCQuadTree *BuildFromBottom(RC_FLOAT bbox[2][2], int resolution[2]);
    
    void Clear();
    
    static void InsertPoints(RCQuadTree *root,
                             const vector<RayCaster2::Point> &points,
                             const vector<RayCaster2::Point2D> &projPoints);
    
    static bool Visit(RCQuadTree *root, const Visitor &visitor);
    
    // Get all the points in a single list
    static void GetPoints(RCQuadTree *root, vector<RayCaster2::Point2D> *points);
    
    // Get all the points, grouped by node
    static void GetPoints(RCQuadTree *root, vector<vector<RayCaster2::Point2D> > *points);
    
    static vector<vector<RCQuadTree *> > &GetLeaves(RCQuadTree *root);
    
protected:
    friend class RCQuadTreeVisitor;
    
    void InsertPoint(const RayCaster2::Point2D &points);
    
    void GetPoints(vector<RayCaster2::Point2D> *points);
    
    void GetPoints(vector<vector<RayCaster2::Point2D> > *points);
    
    bool IsLeaf();
    
    bool Visit(const Visitor &visitor);
    
    //
    RCQuadTree *mChildren[4];

    RC_FLOAT mBBox[2][2];
    
    vector<RayCaster2::Point2D> mPoints;
    
    int mLevel;
    
    // Keep an array of all the leaves, attached to the root
    vector<vector<RCQuadTree *> > mLeaves;
    
    // To set or get flags (used by visitors)
    int mFlags;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_StereoViz__QuadTree__) */
