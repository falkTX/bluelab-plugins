#ifndef PLUG_BYPASS_DETECTOR_H
#define PLUG_BYPASS_DETECTOR_H

// No so many host seem to support detection of plug bypass
// So use a custom object, to be touched in ProcessBlock(),
// and to be asked in OnIdle()
class PlugBypassDetector
{
 public:
    // 200 ms should be ok for 22050Hz, buffer size 2048
    PlugBypassDetector(int delayMs = 200);
    virtual ~PlugBypassDetector();

    void Touch();
    bool PlugIsBypassed();

 protected:
    int mDelayMs;
    
    long int mPrevTouchTime;
};

#endif
