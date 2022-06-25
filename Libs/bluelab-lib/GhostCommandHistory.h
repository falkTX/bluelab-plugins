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
