//
//  RCQuadTreeVisitor.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 4/6/20.
//
//

#ifdef IGRAPHICS_NANOVG

#include "RCQuadTreeVisitor.h"

// Flag
#define FLAG_OUTSIDE_BOUNDARY 0x1

// Propagate boundary flag to parent ?
// (not sure what is the best value)
#define PROPAGATE_BOUNDARY_PARENT 1


RCQuadTreeVisitor::RCQuadTreeVisitor()
{
    mMaxSize = -1.0;
}

RCQuadTreeVisitor::~RCQuadTreeVisitor() {}

void
RCQuadTreeVisitor::SetMaxSize(RC_FLOAT maxSize)
{
    mMaxSize = maxSize;
}

// If at least one child is empty, transfert all the points to the parent node
// This is to avoid black holes in the rendering
bool
RCQuadTreeVisitor::Visit(RCQuadTree *node) const
{
    //if (node->IsLeaf())
    //    return false;
    
    // Compute the center and size of the current node
    RC_FLOAT centerX = (node->mBBox[0][0] + node->mBBox[1][0])/2.0;
    RC_FLOAT centerY = (node->mBBox[0][1] + node->mBBox[1][1])/2.0;
    RC_FLOAT size = node->mBBox[1][0] - node->mBBox[0][0];
    
    // First, for leaves, set center and size
    //
    if (node->IsLeaf())
    {
        for (int j = 0; j < node->mPoints.size(); j++)
        {
            RayCaster2::Point2D &p = node->mPoints[j];
            
            // Update the point values to the node bounding box
            //p.mXi = centerX;
            //p.mYi = centerY;
            
            p.mX = centerX;
            p.mY = centerY;
            
            p.mSizei[0] = size;
            p.mSizei[1] = size;
        }
        
        return false;
    }
    
    // Avoid that we process too big nodes
    // (otherwise we will often have a single big colored rectangle on the screen)
    if (mMaxSize >= 0.0)
    {
        RC_FLOAT size0 = node->mBBox[1][0] - node->mBBox[0][0];
        if (size0 > mMaxSize)
            return false;
    }
    
    // Boundaries
    for (int i = 0; i < 4; i++)
    {
        if ((node->mChildren[i] != NULL) &&
            //node->mChildren[i]->IsLeaf() &&
            (node->mChildren[i]->mFlags & FLAG_OUTSIDE_BOUNDARY))
            // Do not process the boundaries
        {
#if PROPAGATE_BOUNDARY_PARENT
            // Propagate to parent ?
            node->mFlags |= FLAG_OUTSIDE_BOUNDARY;
#endif
            
            return false;
        }
    }
    
    // At least one filled child
    int numFilledChildren = 0;
    // At least one empty child
    int numEmptyChildren = 0;
    for (int i = 0; i < 4; i++)
    {
        if (!node->mChildren[i]->mPoints.empty())
        {
            numFilledChildren++;
        }
        
        if (node->mChildren[i]->mPoints.empty())
        {
            numEmptyChildren++;
        }
    }
    
    // Do we need to process ?
    //
#if 1
    // We process only if we have both some empty and some filled nodes
    //
    if ((numEmptyChildren == 4) || (numFilledChildren == 4))
        return false;
#endif
    
#if 0 // Doesn't work well...
    // Test to try to manage the boudary
    // (otherwise we will have big squares all over the boundary
    if ((numEmptyChildren > 2) || (numFilledChildren == 4))
        return false;
#endif
    
    // We need to process!
    //
    
    // Put the children points to the current node
    // And update their values
    for (int i = 0; i < 4; i++)
    {
        if ((node->mChildren[i] != NULL) && (!node->mChildren[i]->mPoints.empty()))
        {
            for (int j = 0; j < node->mChildren[i]->mPoints.size(); j++)
            {
                RayCaster2::Point2D &p = node->mChildren[i]->mPoints[j];
                
                // Update the point values to the node bounding box
                //p.mXi = centerX;
                //p.mYi = centerY;
                p.mX = centerX;
                p.mY = centerY;
                
                p.mSizei[0] = size;
                p.mSizei[1] = size;
                
                // Add the point to the current node
                // May be optimized...
                node->mPoints.push_back(p);
            }
            
            // Remove the points from the child
            node->mChildren[i]->mPoints.clear();
        }
    }
    
    return true;
}

void
RCQuadTreeVisitor::TagBoundaries(RCQuadTree *root)
{
#define BOUNDARY_SIZE 1 //8 //4 //1 //4 //1 //2
    
    vector<vector<RCQuadTree *> > &leaves = RCQuadTree::GetLeaves(root);
    
    for (int i = 0; i < leaves.size(); i++)
    {
        for (int j = 0; j < leaves[i].size(); j++)
        {
            bool inside = false;
            
            for (int i0 = i - BOUNDARY_SIZE; i0 < i + BOUNDARY_SIZE; i0++)
            {
                for (int j0 = j - BOUNDARY_SIZE; j0 < j + BOUNDARY_SIZE; j0++)
                {
                    if ((i0 >= 0) && (i0 <= leaves.size() - 1) &&
                        (j0 >= 0) && (j0 <= leaves[i0].size() - 1))
                    {
                        if (!leaves[i0][j0]->mPoints.empty())
                        {
                            inside = true;
                            
                            break;
                        }
                    }
                }
            }
            
            // Set the flag if necessary
            leaves[i][j]->mFlags = FLAG_OUTSIDE_BOUNDARY;
            if (inside)
            {
                leaves[i][j]->mFlags = 0;
            }
        }
    }
}

#endif // IGRAPHICS_NANOVG
