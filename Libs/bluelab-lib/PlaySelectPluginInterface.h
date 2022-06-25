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
 
#ifndef PLAY_SELECT_PLUGIN_INTERFACE_H
#define PLAY_SELECT_PLUGIN_INTERFACE_H

class PlaySelectPluginInterface
{
 public:
    // Play bar
    virtual void SetBarActive(bool flag) = 0;
    virtual bool IsBarActive() = 0;
    virtual void SetBarPos(BL_FLOAT x) = 0;
    virtual BL_FLOAT GetBarPos() = 0;
    virtual void ResetPlayBar() = 0;
    virtual void ClearBar() = 0;
    
    virtual void StartPlay() = 0;
    virtual void StopPlay() = 0;
    virtual bool PlayStarted() = 0;
    
    // Selection
    virtual bool IsSelectionActive() = 0;
    virtual void UpdateSelection(BL_FLOAT x0, BL_FLOAT y0,
                                 BL_FLOAT x1, BL_FLOAT y1,
                                 bool updateCenterPos,
                                 bool activateDrawSelection = false,
                                 bool updateCustomControl = false) = 0;
    virtual void SelectionChanged() = 0;
    virtual bool PlayBarOutsideSelection() = 0;

    virtual void BarSetSelection(int x) = 0;
    virtual void SetSelectionActive(bool flag) = 0;
    
    virtual void UpdatePlayBar() = 0;

    //
    virtual void GetGraphSize(int *width, int *height) = 0;
};

#endif
