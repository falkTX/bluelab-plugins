//
//  RCKdTree.cpp
//  BL-StereoViz
//
//  Created by applematuer on 11/17/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLUtils.h>

#include "RCKdTree.h"

#define INF 1e15


RCKdTree::Visitor::Visitor() {}

RCKdTree::Visitor::~Visitor() {}

//

RCKdTree::~RCKdTree()
{
    for (int i = 0; i < 2; i++)
    {
        if (mChildren[i] != NULL)
            delete mChildren[i];
    }
}

RCKdTree *
RCKdTree::Build(int maxLevel)
{
    RCKdTree *root = new RCKdTree(0);
    
    vector<RCKdTree *> trees;
    trees.push_back(root);
    
    int level = 1;
    while(level < maxLevel)
    {
        vector<RCKdTree *> childTrees;
        
        for (int i = 0; i < trees.size(); i++)
        {
            RCKdTree *tree = trees[i];
            
            for (int j = 0; j < 2; j++)
            {
                RCKdTree *child = new RCKdTree(level);
                
                tree->mChildren[j] = child;
                
                childTrees.push_back(child);
            }
        }
        
        trees = childTrees;
        
        level++;
    }
    
    return root;
}

void
RCKdTree::Clear(RCKdTree *root)
{
    root->Clear();
}

void
RCKdTree::InsertPoints(RCKdTree *root,
                       const vector<RayCaster2::Point> &points,
                       const vector<RayCaster2::Point2D> &projPoints,
                       SplitMethod method)
{
    // Initialize the root
    
    // Compute root bounding box
    RC_FLOAT bbox[2][2];
    bbox[0][0] = INF;
    bbox[0][1] = INF;
    
    bbox[1][0] = -INF;
    bbox[1][1] = -INF;
    
    vector<RayCaster2::Point2D> pointsToInsert;
    for (int i = 0; i < points.size(); i++)
    {
        const RayCaster2::Point &p = points[i];
        
        if ((p.mFlags & RC_POINT_FLAG_DISCARDED_THRS) ||
            (p.mFlags & RC_POINT_FLAG_DISCARDED_CLIP))
            continue;
        
        const RayCaster2::Point2D &p2 = projPoints[i];
        pointsToInsert.push_back(p2);
        
        if (p2.mX/*mXi*/ < bbox[0][0])
            bbox[0][0] = p2.mX/*mXi*/;
        if (p2.mY/*mYi*/ < bbox[0][1])
            bbox[0][1] = p2.mY/*mYi*/;
        
        if (p2.mX/*mXi*/ > bbox[1][0])
            bbox[1][0] = p2.mX/*mXi*/;
        if (p2.mY/*mYi*/ > bbox[1][1])
            bbox[1][1] = p2.mY/*mYi*/;
    }
    
    root->InsertPoints(bbox, pointsToInsert, method);
}

bool
RCKdTree::Visit(RCKdTree *root, const Visitor &visitor)
{
    bool result = root->Visit(visitor);
        
    return result;
}

void
RCKdTree::GetPoints(RCKdTree *root, vector<RayCaster2::Point2D> *points)
{
    points->clear();
    
    root->GetPoints(points);
}

void
RCKdTree::GetPoints(RCKdTree *root, vector<vector<RayCaster2::Point2D> > *points)
{
    points->clear();
    
    root->GetPoints(points);
}

RCKdTree::RCKdTree(int level)
{
    mBBox[0][0] = 0.0;
    mBBox[0][1] = 0.0;
    mBBox[1][0] = 0.0;
    mBBox[1][1] = 0.0;
    
    for (int i = 0; i < 2; i++)
    {
        mChildren[i] = NULL;
    }
    
    mLevel = level;
}

void
RCKdTree::Clear()
{
    for (int i = 0; i < 2; i++)
    {
        if (mChildren[i] != NULL)
            mChildren[i]->Clear();
    }
    
    mPoints.clear();
}

void
RCKdTree::InsertPoints(RC_FLOAT bbox[2][2],
                       const vector<RayCaster2::Point2D> &projPoints,
                       SplitMethod method)
{
    // Set bounding box
    mBBox[0][0] = bbox[0][0];
    mBBox[0][1] = bbox[0][1];
    
    mBBox[1][0] = bbox[1][0];
    mBBox[1][1] = bbox[1][1];
    
    if (IsLeaf()) // || (points.size() < 2))
    {
        mPoints = projPoints;
        
        return;
    }
    
    // Split and transmit to the children
    RC_FLOAT bbox2[2][2][2];
    vector<RayCaster2::Point2D> points2[2];
    
    if (method == SPLIT_METHOD_MEDIAN)
        SplitMedian(bbox, projPoints, bbox2, points2);
    else if (method == SPLIT_METHOD_MIDDLE)
        SplitMiddle(bbox, projPoints, bbox2, points2);
    
    // Finally update the children
    for (int i = 0; i < 2; i++)
        mChildren[i]->InsertPoints(bbox2[i], points2[i], method);
}

void
RCKdTree::GetPoints(vector<RayCaster2::Point2D> *points)
{
    for (int i = 0; i < mPoints.size(); i++)
        points->push_back(mPoints[i]);
    
    for (int i = 0; i < 2; i++)
    {
        if (mChildren[i] != NULL)
        {
            mChildren[i]->GetPoints(points);
        }
    }
}

void
RCKdTree::GetPoints(vector<vector<RayCaster2::Point2D> > *points)
{
    if (!mPoints.empty())
        points->push_back(mPoints);
    
    for (int i = 0; i < 2; i++)
    {
        if (mChildren[i] != NULL)
        {
            mChildren[i]->GetPoints(points);
        }
    }
}

bool
RCKdTree::IsLeaf()
{
    for (int i = 0; i < 2; i++)
    {
        if (mChildren[i] != NULL)
            return false;
    }
    
    return true;
}

bool
RCKdTree::Visit(const Visitor &visitor)
{
    bool result = false;
    
    for (int i = 0; i < 2; i++)
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

void
RCKdTree::SplitMedian(const RC_FLOAT bbox[2][2],
                      const vector<RayCaster2::Point2D> &points,
                      RC_FLOAT bbox2[2][2][2],
                      vector<RayCaster2::Point2D> points2[2])
{
    if (bbox[1][0] - bbox[0][0] > bbox[1][1] - bbox[0][1])
        // X is the biggest dimension
    {
        // Take the middle of the number of points
        
        RC_FLOAT middleX = 0.0;
        for (int i = 0; i < points.size(); i++)
        {
            const RayCaster2::Point2D &p = points[i];
            middleX += p.mX/*mXi*/;
        }
        
        if (!points.empty())
            middleX /= points.size();
        
        // Bounding boxes
        //
        
        // For first child
        bbox2[0][0][0] = bbox[0][0];
        bbox2[0][1][0] = middleX;
        
        bbox2[0][0][1] = bbox[0][1];
        bbox2[0][1][1] = bbox[1][1];
        
        // For second child
        bbox2[1][0][0] = middleX;
        bbox2[1][1][0] = bbox[1][0];
        
        bbox2[1][0][1] = bbox[0][1];
        bbox2[1][1][1] = bbox[1][1];
        
        // Points
        //
        for (int i = 0; i < points.size(); i++)
        {
            const RayCaster2::Point2D &p = points[i];
            if (p.mX/*mXi*/ < middleX)
            {
                points2[0].push_back(p);
            }
            else
            {
                points2[1].push_back(p);
            }
        }
    }
    else
    {
        // Take the middle of the number of points
        RC_FLOAT middleY = 0.0;
        for (int i = 0; i < points.size(); i++)
        {
            const RayCaster2::Point2D &p = points[i];
            middleY += p.mY/*mYi*/;
        }
        
        if (!points.empty())
            middleY /= points.size();
        
        // Bounding boxes
        //
        
        // For first child
        bbox2[0][0][0] = bbox[0][0];
        bbox2[0][1][0] = bbox[1][0];
        
        bbox2[0][0][1] = bbox[0][1];
        bbox2[0][1][1] = middleY;
        
        // For second child
        bbox2[1][0][0] = bbox[0][0];
        bbox2[1][1][0] = bbox[1][0];
        
        bbox2[1][0][1] = middleY;
        bbox2[1][1][1] = bbox[1][1];
        
        // Points
        //
        for (int i = 0; i < points.size(); i++)
        {
            const RayCaster2::Point2D &p = points[i];
            if (p.mY/*mYi*/ < middleY)
            {
                points2[0].push_back(p);
            }
            else
            {
                points2[1].push_back(p);
            }
        }
    }
}

void
RCKdTree::SplitMiddle(const RC_FLOAT bbox[2][2],
                      const vector<RayCaster2::Point2D> &points,
                      RC_FLOAT bbox2[2][2][2],
                      vector<RayCaster2::Point2D> points2[2])
{
    if (bbox[1][0] - bbox[0][0] > bbox[1][1] - bbox[0][1])
        // X is the biggest dimension
    {
        // Take the middle of the cell
        RC_FLOAT middleX = (bbox[0][0] + bbox[1][0])*0.5;
        
        // Bounding boxes
        //
        
        // For first child
        bbox2[0][0][0] = bbox[0][0];
        bbox2[0][1][0] = middleX;
        
        bbox2[0][0][1] = bbox[0][1];
        bbox2[0][1][1] = bbox[1][1];
        
        // For second child
        bbox2[1][0][0] = middleX;
        bbox2[1][1][0] = bbox[1][0];
        
        bbox2[1][0][1] = bbox[0][1];
        bbox2[1][1][1] = bbox[1][1];
        
        // Points
        //
        for (int i = 0; i < points.size(); i++)
        {
            const RayCaster2::Point2D &p = points[i];
            if (p.mX/*mXi*/ < middleX)
            {
                points2[0].push_back(p);
            }
            else
            {
                points2[1].push_back(p);
            }
        }
    }
    else
    {
        // Take the middle of the cell
        RC_FLOAT middleY = (bbox[0][1] + bbox[1][1])*0.5;
        
        // Bounding boxes
        //
        
        // For first child
        bbox2[0][0][0] = bbox[0][0];
        bbox2[0][1][0] = bbox[1][0];
        
        bbox2[0][0][1] = bbox[0][1];
        bbox2[0][1][1] = middleY;
        
        // For second child
        bbox2[1][0][0] = bbox[0][0];
        bbox2[1][1][0] = bbox[1][0];
        
        bbox2[1][0][1] = middleY;
        bbox2[1][1][1] = bbox[1][1];
        
        // Points
        //
        for (int i = 0; i < points.size(); i++)
        {
            const RayCaster2::Point2D &p = points[i];
            if (p.mY/*mYi*/ < middleY)
            {
                points2[0].push_back(p);
            }
            else
            {
                points2[1].push_back(p);
            }
        }
    }
}

#endif //IGRAPHICS_NANOVG
