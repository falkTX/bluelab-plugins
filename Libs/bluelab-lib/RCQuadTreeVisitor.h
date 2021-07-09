//
//  RCQuadTreeVisitor.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 4/6/20.
//
//

#ifndef __BL_SoundMetaViewer__RCQuadTreeVisitor__
#define __BL_SoundMetaViewer__RCQuadTreeVisitor__

#ifdef IGRAPHICS_NANOVG

#include <RCQuadTree.h>

class RCQuadTreeVisitor : public RCQuadTree::Visitor
{
public:
    RCQuadTreeVisitor();
    
    virtual ~RCQuadTreeVisitor();
    
    void SetMaxSize(RC_FLOAT maxSize);
    
    bool Visit(RCQuadTree *node) const override;
    
    static void TagBoundaries(RCQuadTree *root);
    
protected:
    RC_FLOAT mMaxSize;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_SoundMetaViewer__RCQuadTreeVisitor__) */
