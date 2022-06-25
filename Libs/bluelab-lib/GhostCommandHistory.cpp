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
