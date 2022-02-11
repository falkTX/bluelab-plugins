#ifndef GHOST_COMMAND_HISTORY_H
#define GHOST_COMMAND_HISTORY_H

#include <deque>
using namespace std;

#include <GhostCommand.h>

#include "IPlug_include_in_plug_hdr.h"

// Command list ("history") for undo mechanism
class GhostCommandHistory
{
public:
    GhostCommandHistory(int maxNumCommands);
    
    virtual ~GhostCommandHistory();
    
    void Clear();
    
    void AddCommand(GhostCommand *command);
    
    // For the moment, no "redo" !
    void UndoLastCommand(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                         vector<WDL_TypedBuf<BL_FLOAT> > *phases);
    
    GhostCommand *GetLastCommand();
    
protected:
    deque<GhostCommand *> mCommands;
    
    int mMaxNumCommands;
};


#endif
