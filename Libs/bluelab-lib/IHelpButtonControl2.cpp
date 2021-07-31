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
