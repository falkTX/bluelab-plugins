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
//  IHelpButtonControl2.cpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/5/20.
//
//

#include "IHelpButtonControl2.h"

void
IHelpButtonControl2::OnMouseDown(float x, float y, const IMouseMod &mod)
{
    ShowManual(mFileName);
    
    IRolloverButtonControl::OnMouseDown(x, y, mod);

    // FIX: on windows, the button stayed blue after clicking on it
    SetValueFromUserInput(0.0);
}

void
IHelpButtonControl2::ShowManual(const char *fileName)
{
#ifdef __APPLE__
    // Don't forget that path can have white spaces inside!
    //char cmd[1024];
    char cmd[2048];

    sprintf(cmd, "open \"%s\"", fileName);
    
    system(cmd);
#endif
    
#ifdef WIN32

#ifndef VST3_API
    ShellExecute(NULL, "open", fileName, NULL, NULL, SW_SHOWNORMAL);
#else
    //ShellExecute(NULL, (LPCWSTR)"open", (LPCWSTR)fileName, NULL, NULL, SW_SHOWNORMAL);
    ShellExecuteA(NULL, "open", fileName, NULL, NULL, SW_SHOWNORMAL);
#endif

#endif

#ifdef __linux__
    // Don't forget that path can have white spaces inside!
    //char cmd[1024];
    char cmd[2048];

    sprintf(cmd, "xdg-open \"%s\"", fileName);
    
    system(cmd);
#endif
}
