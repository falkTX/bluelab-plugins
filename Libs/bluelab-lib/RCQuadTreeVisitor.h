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
