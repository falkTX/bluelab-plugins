//
//  RCKdTreeVisitor.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 4/6/20.
//
//

#ifndef __BL_SoundMetaViewer__RCKdTreeVisitor__
#define __BL_SoundMetaViewer__RCKdTreeVisitor__

#ifdef IGRAPHICS_NANOVG

#include <RCKdTree.h>

class RCKdTreeVisitor : public RCKdTree::Visitor
{
public:
    RCKdTreeVisitor();
    
    virtual ~RCKdTreeVisitor();
    
    bool Visit(RCKdTree *node) const override;
    
    void SetMaxSize(RC_FLOAT maxSize);
    
    //static void TagBoundaries(RCKdTree *root);
    
protected:
    RC_FLOAT mMaxSize;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__BL_SoundMetaViewer__RCKdTreeVisitor__) */
