//
//  GraphGroup.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 02/10/17.
//
//

#include "GraphControl2.h"

#include "GraphGroup.h"

GraphGroup::GraphGroup(IPlugBase *pPlug, IRECT pR,
                       GraphControl2 *graph0, GraphControl2 *graph1)
: IControl(pPlug, pR)
{
    mGraphs[0] = graph0;
    mGraphs[1] = graph1;
}

GraphGroup::~GraphGroup()
{
    delete mGraphs[0];
    delete mGraphs[1];
}

bool
GraphGroup::IsDirty()
{
    bool dirty = (mGraphs[0]->IsDirty() && mGraphs[1]->IsDirty());
    
    return dirty;
}

bool
GraphGroup::Draw(IGraphics *pGraphics)
{
    IPlugBase::IMutexLock lock(mPlug);
    
    bool draw0 = mGraphs[0]->Draw(pGraphics);
    bool draw1 = mGraphs[1]->Draw(pGraphics);
    
    bool result = (draw0 && draw1);
    
    return result;
}

void
GraphGroup::GetGroupRect(IRECT *groupRect, IControl *control0, IControl *control1)
{
    *groupRect = *control0->GetRECT();
    
    const IRECT *rect1 = control1->GetRECT();
    
    *groupRect = groupRect->Union((IRECT *)rect1);
}