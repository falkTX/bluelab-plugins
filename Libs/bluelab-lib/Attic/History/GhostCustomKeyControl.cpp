#include "GhostCustomKeyControl.h"

GhostCustomKeyControl::GhostCustomKeyControl(GhostPluginInterface *plug,
                                             const IRECT &bounds)
: IControl(bounds)
{
    mPlug = plug;
    
    SetInteractionDisabled(true);
}

bool
GhostCustomKeyControl::OnKeyDown(float x, float y, const IKeyPress &key)
{
#if !APP_API
    // Non-standalone version
    // => Disable keyboard shortcuts on plugin versions !
    
    return false;
#endif
    
#if 0
    fprintf(stderr, "key: %d\n", key.VK);
#endif
    
    if (key.VK == 0)
        // Spacebar
    {
        if (!mPlug->PlayStarted())
        {
            mPlug->StartPlay();

	    // TODO
	    
            // Synchronize the play button state
            //mPlug->GetGUI()->SetParameterFromPlug(kPlayStop, 1, false);
            mPlug->SetPlayStopParameter(1);
        }
        else
        {
            mPlug->StopPlay();
            
            // Synchronize the play button state
            //mPlug->GetGUI()->SetParameterFromPlug(kPlayStop, 0, false);
            mPlug->SetPlayStopParameter(0);
        }
        
        return true;
    }
    
    if (key.VK == 41)
        // Return
    {
        mPlug->RewindView();
    }
    
    if ((key.VK == 38) && key.C) // cmd-x
    {
        mPlug->DoCutCommand();
    }
    
    if ((key.VK == 16) && key.C) // cmd-b
    {
        mPlug->DoGainCommand();
    }
    
#if !GHOST_LITE_VERSION || GHOST_LITE_ENABLE_REPLACE
    // cmd-w was not transmitted here
    if ((key.VK == 28) && key.C) // cmd-n
    {
        mPlug->DoReplaceCommand();
    }
#endif
    
#if !GHOST_LITE_VERSION || GHOST_LITE_ENABLE_COPY_PASTE
    if ((key.VK == 17) && key.C) // cmd-c
    {
        mPlug->DoCopyCommand();
    }
    
    if ((key.VK == 36) && key.C) // cmd-v
    {
        mPlug->DoPasteCommand();
    }
#endif
    
    if ((key.VK == 40) && key.C) // cmd-z
    {
        mPlug->UndoLastCommand();
    }
    
    return true;
}
