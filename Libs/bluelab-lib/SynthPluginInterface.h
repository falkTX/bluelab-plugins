#ifndef SYNTH_PLUGIN_INTERFACE_H
#define SYNTH_PLUGIN_INTERFACE_H

class SynthPluginInterface
{
 public:
    virtual int GetNumKeys() = 0;
    virtual bool GetKeyStatus(int key) = 0;
};

#endif
