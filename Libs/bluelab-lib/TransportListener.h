#ifndef TRANSPORT_LISTENER_H
#define TRANSPORT_LISTENER_H

class TransportListener
{
 public:
    virtual void TransportPlayingChanged() = 0;
};

#endif
