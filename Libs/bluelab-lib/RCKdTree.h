//
//  RCKdTree.h
//  BL-StereoViz
//
//  Created by applematuer on 11/17/18.
//
//

#ifndef __BL_StereoViz__RCKdTree__
#define __BL_StereoViz__RCKdTree__

#ifdef IGRAPHICS_NANOVG

#include <vector>
using namespace std;

#include <RayCaster2.h>


// Kd-tree for RayCaster (1 or 2)

class RCKdTree
{
public:
    class Visitor
    {
    public:
        Visitor();
        
        virtual ~Visitor();
        
        virtual bool Visit(RCKdTree *node) const = 0;
    };
    
    enum SplitMethod
    {
        SPLIT_METHOD_MEDIAN,
        SPLIT_METHOD_MIDDLE
    };
    
    //
    virtual ~RCKdTree();
    
    static RCKdTree *Build(int maxLevel);
    
    static void Clear(RCKdTree *root);
    
    static void InsertPoints(RCKdTree *root,
                             const vector<RayCaster2::Point> &points,
                             const vector<RayCaster2::Point2D> &projPoints,
                             SplitMethod method);
    
    static bool Visit(RCKdTree *root, const Visitor &visitor);
    
    // Get all the points in a single list
    static void GetPoints(RCKdTree *root, vector<RayCaster2::Point2D> *points);
    
    // Get all the points, grouped by node
    static void GetPoints(RCKdTree *root, vector<vector<RayCaster2::Point2D> > *points);
    
protected:
    RCKdTree(int level);
    
    friend class RCKdTreeVisitor;
    
    void Clear();
    
    void InsertPoints(RC_FLOAT bbox[2][2],
                      const vector<RayCaster2::Point2D> &projPoints,
                      SplitMethod method);
    
    void GetPoints(vector<RayCaster2::Point2D> *points);
    
    void GetPoints(vector<vector<RayCaster2::Point2D> > *points);
    
    bool IsLeaf();
    
    bool Visit(const Visitor &visitor);
    
    // Split method median: divide the number of points by 2
    static void SplitMedian(const RC_FLOAT bbox[2][2],
                            const vector<RayCaster2::Point2D> &points,
                            RC_FLOAT bbox2[2][2][2],
                            vector<RayCaster2::Point2D> points2[2]);
    
    // Split method middle: divide the space  by 2
    static void SplitMiddle(const RC_FLOAT bbox[2][2],
                            const vector<RayCaster2::Point2D> &points,
                            RC_FLOAT bbox2[2][2][2],
                            vector<RayCaster2::Point2D> points2[2]);
    
    //
    RCKdTree *mChildren[2];

    RC_FLOAT mBBox[2][2];
    
    vector<RayCaster2::Point2D> mPoints;
    
    int mLevel;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_StereoViz__QuadTree__) */
