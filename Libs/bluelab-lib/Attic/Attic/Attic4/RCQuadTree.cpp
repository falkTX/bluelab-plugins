//
//  RCQuadTree.cpp
//  BL-StereoViz
//
//  Created by applematuer on 11/17/18.
//
//

#include <BLUtils.h>

#include "RCQuadTree.h"

RCQuadTree::Visitor::Visitor() {}

RCQuadTree::Visitor::~Visitor() {}

//

RCQuadTree::RCQuadTree(RC_FLOAT bbox[2][2], int level)
{
    mBBox[0][0] = bbox[0][0];
    mBBox[0][1] = bbox[0][1];
    
    mBBox[1][0] = bbox[1][0];
    mBBox[1][1] = bbox[1][1];
    
    for (int i = 0; i < 4; i++)
    {
        mChildren[i] = NULL;
    }
    
    mLevel = level;
    
    mFlags = 0;
}

RCQuadTree::~RCQuadTree()
{
    for (int i = 0; i < 4; i++)
    {
        if (mChildren[i] != NULL)
            delete mChildren[i];
    }
}

RCQuadTree *
RCQuadTree::BuildFromBottom(RC_FLOAT bbox[2][2], int resolution[2])
{
    RCQuadTree *root = NULL;
    
    resolution[0] = BLUtils::NextPowerOfTwo(resolution[0]);
    resolution[1] = BLUtils::NextPowerOfTwo(resolution[1]);
    
    int numTrees = resolution[0]*resolution[1];
    
    RC_FLOAT cellSize = (bbox[1][0] - bbox[0][0])/resolution[0];//1.0;
    
    //
    vector<vector<RCQuadTree *> > rootLeaves;
    
    //
    vector<vector<RCQuadTree *> > prevTrees;
    
    int level = 0;
    while(numTrees >= 1)
    {
        vector<vector<RCQuadTree *> > trees;
        trees.resize(resolution[0]);
        for (int i = 0; i < resolution[0]; i++)
        {
            trees[i].resize(resolution[1]);
            
            for (int j = 0; j < resolution[1]; j++)
            {
                RC_FLOAT tbox[2][2];
                tbox[0][0] = bbox[0][0] + i*cellSize;
                tbox[0][1] = bbox[0][1] + j*cellSize;
                
                tbox[1][0] = bbox[0][0] + (i + 1)*cellSize;
                tbox[1][1] = bbox[0][1] + (j + 1)*cellSize;
                
                RCQuadTree *tree = new RCQuadTree(tbox, level);
                
                if (!prevTrees.empty())
                {
                    tree->mChildren[0] = prevTrees[i*2][j*2];
                    tree->mChildren[1] = prevTrees[i*2 + 1][j*2];
                    tree->mChildren[2] = prevTrees[i*2][j*2 + 1];
                    tree->mChildren[3] = prevTrees[i*2 + 1][j*2 + 1];
                }
                
                trees[i][j] = tree;
            }
        }
        
        if (numTrees <= 1)
        {
            root = trees[0][0];
        }
        
        cellSize *= 2.0;
        numTrees /= 4;
        
        resolution[0] /= 2;
        resolution[1] /= 2;
        
        prevTrees = trees;
        
        // Keep an array of all the leaves in the root
        if (level == 0)
        {
            rootLeaves = trees;
        }
        
        // Got 1 step up
        level++;
    }
    
    root->mLeaves = rootLeaves;
    
    return root;
}

void
RCQuadTree::Clear()
{
    for (int i = 0; i < 4; i++)
    {
        if (mChildren[i] != NULL)
            mChildren[i]->Clear();
    }
    
    mPoints.clear();
}

// NOTE: not optimized !
// TODO: optimize by adding points in a flat manner
void
RCQuadTree::InsertPoints(RCQuadTree *root,
                         const vector<RayCaster2::Point> &points,
                         const vector<RayCaster2::Point2D> &projPoints)
{
    for (int i = 0; i < points.size(); i++)
    {
        const RayCaster2::Point &p = points[i];
        
        if ((p.mFlags & RC_POINT_FLAG_DISCARDED_THRS) ||
            (p.mFlags & RC_POINT_FLAG_DISCARDED_CLIP))
            continue;
        
        const RayCaster2::Point2D &p2 = projPoints[i];
        
        root->InsertPoint(p2);
    }
}

bool
RCQuadTree::Visit(RCQuadTree *root, const Visitor &visitor)
{
    bool result = root->Visit(visitor);
        
    return result;
}

void
RCQuadTree::GetPoints(RCQuadTree *root, vector<RayCaster2::Point2D> *points)
{
    points->clear();
    
    root->GetPoints(points);
}

void
RCQuadTree::GetPoints(RCQuadTree *root, vector<vector<RayCaster2::Point2D> > *points)
{
    points->clear();
    
    root->GetPoints(points);
}

void
RCQuadTree::InsertPoint(const RayCaster2::Point2D &point)
{
    bool inserted = false;
    for (int i = 0; i < 4; i++)
    {
        if (mChildren[i] != NULL)
        {
            if ((point.mX/*mXi*/ >= mChildren[i]->mBBox[0][0]) &&
                (point.mX/*mXi*/ <= mChildren[i]->mBBox[1][0]) &&
                (point.mY/*mYi*/ >= mChildren[i]->mBBox[0][1]) &&
                (point.mY/*mYi*/ <= mChildren[i]->mBBox[1][1]))
            {
                mChildren[i]->InsertPoint(point);
                
                inserted = true;
                
                break;
            }
        }
    }
    
    if (IsLeaf())
        mPoints.push_back(point);
}

void
RCQuadTree::GetPoints(vector<RayCaster2::Point2D> *points)
{
    for (int i = 0; i < mPoints.size(); i++)
        points->push_back(mPoints[i]);
    
    for (int i = 0; i < 4; i++)
    {
        if (mChildren[i] != NULL)
        {
            mChildren[i]->GetPoints(points);
        }
    }
}

void
RCQuadTree::GetPoints(vector<vector<RayCaster2::Point2D> > *points)
{
    if (!mPoints.empty())
        points->push_back(mPoints);
    
    for (int i = 0; i < 4; i++)
    {
        if (mChildren[i] != NULL)
        {
            mChildren[i]->GetPoints(points);
        }
    }
}

bool
RCQuadTree::IsLeaf()
{
    for (int i = 0; i < 4; i++)
    {
        if (mChildren[i] != NULL)
            return false;
    }
    
    return true;
}

bool
RCQuadTree::Visit(const Visitor &visitor)
{
    bool result = false;
    
    for (int i = 0; i < 4; i++)
    {
        if (mChildren[i] != NULL)
        {
            bool result0 = mChildren[i]->Visit(visitor);
            if (result0)
                result = true;
        }
    }
    
    // Visit children first
    visitor.Visit(this);
    
    return result;
}

vector<vector<RCQuadTree *> > &
RCQuadTree::GetLeaves(RCQuadTree *root)
{
    return root->mLeaves;
}
