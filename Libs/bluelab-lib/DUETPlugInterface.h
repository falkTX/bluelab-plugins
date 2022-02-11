//
//  DUETPlugInterface.h
//  UST-macOS
//
//  Created by applematuer on 9/26/20.
//
//

#ifndef DUETPlugInterface_h
#define DUETPlugInterface_h

class DUETPlugInterface
{
public:
    virtual void SetPickCursorActive(bool flag) = 0;
    
    virtual void SetPickCursor(int x, int y) = 0;
    
    virtual void SetInvertPickSelection(bool flag) = 0;
};

#endif /* DUETPlugInterface_h */
