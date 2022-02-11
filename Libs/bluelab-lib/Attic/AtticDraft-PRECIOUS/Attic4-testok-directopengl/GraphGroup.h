//
//  GraphGroup.h
//  Transient
//
//  Created by Apple m'a Tuer on 02/10/17.
//
//

#ifndef Transient_GraphGroup_h
#define Transient_GraphGroup_h

#include "../../WDL/IPlug/IControl.h"

class GraphControl2;

// Graph group, for drawing two graph controls synchronously

// Two graphs for the moment
class GraphGroup : public IControl
{
public:
    GraphGroup(IPlugBase *pPlug, IRECT pR, GraphControl2 *graph0, GraphControl2 *graph1);
    
    virtual ~GraphGroup();
    
    bool IsDirty();
    
    bool Draw(IGraphics *pGraphics);
    
    static void GetGroupRect(IRECT *groupRect, IControl *control0, IControl *control1);
    
protected:
    GraphControl2 *mGraphs[2];
};

#endif
