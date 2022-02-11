#ifndef EQHACK_PLUGIN_INTERFACE_H
#define EQHACK_PLUGIN_INTERFACE_H

#include <BLTypes.h>

class EQHackPluginInterface
{
public:
    enum Mode
    {
        LEARN = 0,
        GUESS = 1,
        RESET = 2,
        APPLY = 3,
        APPLY_INV = 4,
    };
};

#endif
