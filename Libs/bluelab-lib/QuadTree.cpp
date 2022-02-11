//
//  QuadTree.cpp
//  BL-StereoViz
//
//  Created by applematuer on 11/17/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "QuadTree.h"

// Cell rendering works
//
// Quad tree is not debugged
//
#define CELL_RENDERING 1

QuadTree::PointMerger::PointMerger() {}

QuadTree::PointMerger::~PointMerger() {}

bool
QuadTree::PointMerger::IsMergeable(const vector<SparseVolRender::Point> &points) const
{
    //if (points.empty())
    //    return false;
    
    //if (points.size() < 2)
    //    return false;
    
    if (points.size() < 2) // TEST
        return true;
        
    for (int i = 0; i < points.size(); i++)
    {
        for (int j = 0; j < points.size(); j++)
        {
            // TODO: optimize this
            // (we make twice too much tests)
            if (i != j)
            {
                bool mergeable = IsMergeable(points[i], points[j]);
                
                if (!mergeable)
                    return false;
            }
        }
    }
    
    return true;
}

//

QuadTree::QuadTree(BL_FLOAT bbox[2][2], int level)
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
}

QuadTree::~QuadTree()
{
    for (int i = 0; i < 4; i++)
    {
        if (mChildren[i] != NULL)
            delete mChildren[i];
    }
}

QuadTree *
QuadTree::BuildFromBottom(BL_FLOAT bbox[2][2], int resolution[2])
{
    QuadTree *root = NULL;
    
    resolution[0] = BLUtilsMath::NextPowerOfTwo(resolution[0]);
    resolution[1] = BLUtilsMath::NextPowerOfTwo(resolution[1]);
    
    int numTrees = resolution[0]*resolution[1];
    
    BL_FLOAT cellSize = (bbox[1][0] - bbox[0][0])/resolution[0];//1.0;
    
    vector<vector<QuadTree *> > prevTrees;
    
    int level = 0;
    
    while(numTrees >= 1)
    {
        vector<vector<QuadTree *> > trees;
        trees.resize(resolution[0]);
        for (int i = 0; i < resolution[0]; i++)
        {
            trees[i].resize(resolution[1]);
            
            for (int j = 0; j < resolution[1]; j++)
            {
                BL_FLOAT tbox[2][2];
                tbox[0][0] = bbox[0][0] + i*cellSize;
                tbox[0][1] = bbox[0][1] + j*cellSize;
                
                tbox[1][0] = bbox[0][0] + (i + 1)*cellSize;
                tbox[1][1] = bbox[0][1] + (j + 1)*cellSize;
                
                QuadTree *tree = new QuadTree(tbox, level);
                
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
        
        level++;
    }
    
    return root;
}

void
QuadTree::Clear()
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
QuadTree::InsertPoints(QuadTree *root, const vector<SparseVolRender::Point> &points)
{
    for (int i = 0; i < points.size(); i++)
    {
        const SparseVolRender::Point &p = points[i];
        
        root->InsertPoint(p);
    }
}

void
QuadTree::MergePoints(QuadTree *root, const PointMerger &merger)
{
    bool merged = false;
    
    // TODO: optimize this
    do
    {
        merged = false;
        root->MergePoints(merger, &merged);
        
#if CELL_RENDERING
        // Make only one iteration
        break;
#endif
        
    } while(merged);
}

void
QuadTree::GetPoints(QuadTree *root, vector<SparseVolRender::Point> *points)
{
    points->clear();
    
    root->GetPoints(points);
}

void
QuadTree::InsertPoint(const SparseVolRender::Point &point)
{
    bool inserted = false;
    for (int i = 0; i < 4; i++)
    {
        if (mChildren[i] != NULL)
        {
            if ((point.mX >= mChildren[i]->mBBox[0][0]) &&
                (point.mX <= mChildren[i]->mBBox[1][0]) &&
                (point.mY >= mChildren[i]->mBBox[0][1]) &&
                (point.mY <= mChildren[i]->mBBox[1][1]))
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
QuadTree::GetPoints(vector<SparseVolRender::Point> *points)
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
QuadTree::MergePoints(const PointMerger &merger, bool *merged)
{
    for (int i = 0; i < 4; i++)
    {
        if (mChildren[i] != NULL)
        {
            bool merged0 = false;
            mChildren[i]->MergePoints(merger, &merged0);
            
            if (merged0)
                *merged = true;
        }
    }
 
    vector<SparseVolRender::Point> childPoints;
    for (int i = 0; i < 4; i++)
    {
        if ((mChildren[i] != NULL) && (!mChildren[i]->mPoints.empty()))
        {
            childPoints.insert(childPoints.end(),
                               mChildren[i]->mPoints.begin(),
                               mChildren[i]->mPoints.end());
        }
    }
    
    *merged = false;
    
    //if (childPoints.empty())
    //    *merged = true;
    
    bool mergeable = merger.IsMergeable(childPoints);
    
#if CELL_RENDERING
    // Always merge
    mergeable = true;
#endif
    if (mergeable)
    {
        if (!childPoints.empty())
            
#if CELL_RENDERING
        if (mLevel == 1) // Render only one level
#endif
        {
            SparseVolRender::Point p;
            merger.Merge(childPoints, &p);
        
            // Clear the children points
            for (int i = 0; i < 4; i++)
            {
                if (mChildren[i] != NULL)
                {
                    mChildren[i]->mPoints.clear();
                }
            }
        
#if CELL_RENDERING
            // Render the full cell
            p.mX = (mBBox[0][0] + mBBox[1][0])/2.0;
            p.mY = (mBBox[0][1] + mBBox[1][1])/2.0;
            p.mSize = mBBox[1][0] - mBBox[0][0];
            
            ////p.mA *= 255.0;
            //
#endif
            mPoints.push_back(p);
        
            *merged = true;
        }
    }
}

// Not used
void
QuadTree::GetBottomNodes(vector<QuadTree *> *bottomNodes)
{
    bool noChildren = true;
    for (int i = 0; i < 4; i++)
    {
        if (mChildren[i] != NULL)
        {
            noChildren = false;
            break;
        }
    }
    
    if (noChildren)
        bottomNodes->push_back(this);
    else
    {
        for (int i = 0; i < 4; i++)
        {
            if (mChildren[i] != NULL)
            {
                mChildren[i]->GetBottomNodes(bottomNodes);
            }
        }
    }
}

bool
QuadTree::IsLeaf()
{
    for (int i = 0; i < 4; i++)
    {
        if (mChildren[i] != NULL)
            return false;
    }
    
    return true;
}

#endif // IGRAPHICS_NANOVG
