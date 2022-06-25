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
 
#ifndef GHOST_PLUGIN_INTERFACE_H
#define GHOST_PLUGIN_INTERFACE_H

#include <BLTypes.h>

#include <PlaySelectPluginInterface.h>

// Zoom on the pointer inside of on the bar (for zoom center)
#define ZOOM_ON_POINTER 1

class GhostPluginInterface : public PlaySelectPluginInterface
{
public:
    enum PlugMode
    {
        VIEW = 0,
        ACQUIRE,
        EDIT,
        RENDER
    };
  
    enum SelectionType
    {
        RECTANGLE = 0,
        HORIZONTAL,
        VERTICAL
    };
    
    GhostPluginInterface();
    virtual ~GhostPluginInterface();

    virtual enum PlugMode GetMode() = 0;

    // Ghost
    virtual void BeforeSelTranslation() = 0;
    virtual void AfterSelTranslation() = 0;
    
    virtual void UpdateZoom(BL_FLOAT zoomChange) = 0;
    virtual void SetZoomCenter(int x) = 0;
    virtual void Translate(int dX) = 0;

    virtual void SetNeedRecomputeData(bool flag) = 0;

    virtual void RewindView() = 0;

    virtual void DoCutCommand() = 0;
    virtual void DoCutCopyCommand() = 0;
    virtual void DoGainCommand() = 0;
    virtual void DoReplaceCommand() = 0;
    virtual void DoReplaceCopyCommand() = 0;
    virtual void DoCopyCommand() = 0;
    virtual void DoPasteCommand() = 0;
    virtual void UndoLastCommand() = 0;

    virtual void CheckRecomputeData() = 0;

    virtual void OpenFile(const char *fileName) = 0;
    
    virtual void SetPlayStopParameter(int value) = 0;
  
    // For Protools
    bool PlaybackWasRestarted(unsigned long long delay);

    virtual void CursorMoved(BL_FLOAT x, BL_FLOAT y) = 0;
    virtual void CursorOut() = 0;
        
protected:
    unsigned long long mPrevUpTime;
};

#endif
