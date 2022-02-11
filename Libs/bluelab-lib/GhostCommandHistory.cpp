#include "GhostCommandHistory.h"

GhostCommandHistory::GhostCommandHistory(int maxNumCommands)
{
    mMaxNumCommands = maxNumCommands;
}

GhostCommandHistory::~GhostCommandHistory()
{
    for (int i = 0; i < mCommands.size(); i++)
    {
        GhostCommand *command = mCommands[i];
        delete command;
    }
}

void
GhostCommandHistory::Clear()
{
    for (int i = 0; i < mCommands.size(); i++)
    {
        GhostCommand *command = mCommands[i];
        delete command;
    }
    
    mCommands.clear();
}

void
GhostCommandHistory::AddCommand(GhostCommand *command)
{
    mCommands.push_back(command);

    if (mCommands.size() > mMaxNumCommands)
        // Pop and destroy the oldest command
    {
        GhostCommand *firstCommand = mCommands[0];
        mCommands.pop_front();
        delete firstCommand;
    }
}

// For the moment, no "redo" !
void
GhostCommandHistory::UndoLastCommand(vector<WDL_TypedBuf<BL_FLOAT> > *magns,
                                     vector<WDL_TypedBuf<BL_FLOAT> > *phases)
{
    if (mCommands.empty())
        return;
    
    GhostCommand *lastCommand = mCommands[mCommands.size() - 1];
    mCommands.pop_back();
    
    lastCommand->Undo(magns, phases);
    
    //delete lastCommand;
}

GhostCommand *
GhostCommandHistory::GetLastCommand()
{
    if (mCommands.empty())
        return NULL;
    
    GhostCommand *lastCommand = mCommands[mCommands.size() - 1];
    
    return lastCommand;
}
