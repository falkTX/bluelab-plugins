//
//  RCKdTreeVisitor.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 4/6/20.
//
//

#ifdef IGRAPHICS_NANOVG

#include "RCKdTreeVisitor.h"

// Flag
//#define FLAG_OUTSIDE_BOUNDARY 0x1

// Propagate boundary flag to parent ?
// (not sure what is the best value)
//#define PROPAGATE_BOUNDARY_PARENT 1

RCKdTreeVisitor::RCKdTreeVisitor()
{
    mMaxSize = 1.0;
}

RCKdTreeVisitor::~RCKdTreeVisitor() {}

bool
RCKdTreeVisitor::Visit(RCKdTree *node) const
{
    // Compute the center and size of the current node
    RC_FLOAT centerX = (node->mBBox[0][0] + node->mBBox[1][0])/2.0;
    RC_FLOAT centerY = (node->mBBox[0][1] + node->mBBox[1][1])/2.0;
    RC_FLOAT size[2] = { node->mBBox[1][0] - node->mBBox[0][0],
                       node->mBBox[1][1] - node->mBBox[0][1] };
    
    RC_FLOAT maxSize = size[0];
    if (size[1] > maxSize)
        maxSize = size[1];
    
    for (int j = 0; j < node->mPoints.size(); j++)
    {
        RayCaster2::Point2D &p = node->mPoints[j];
        
        // Dicard on too big size ?
        if (maxSize > mMaxSize)
        {
            // Disable the point
            p.mSizei[0] = 0;
            p.mSizei[1] = 0;
            
            continue;
        }
        
        // Update the point values to the node bounding box
        //p.mXi = centerX;
        //p.mYi = centerY;
        
        p.mX = centerX;
        p.mY = centerY;
        
        p.mSizei[0] = size[0];
        p.mSizei[1] = size[1];
    }
    
    if (node->mPoints.empty())
        return false;;
    
    return true;
}

void
RCKdTreeVisitor::SetMaxSize(RC_FLOAT maxSize)
{
    mMaxSize = maxSize;
}

#if 0
void
RCKdTreeVisitor::TagBoundaries(RCQuadTree *root)
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
#endif

#endif // IGRAPHICS_NANOVG
