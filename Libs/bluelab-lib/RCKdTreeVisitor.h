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
