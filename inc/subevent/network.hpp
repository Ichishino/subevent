#ifndef SUBEVENT_NETWORK_HPP
#define SUBEVENT_NETWORK_HPP

#include <subevent/std.hpp>

SEV_NS_BEGIN

class Thread;
class SocketController;

//---------------------------------------------------------------------------//
// Network
//---------------------------------------------------------------------------//

class Network
{
public:
    SEV_DECL static bool init(Thread* thread);

    SEV_DECL static uint32_t getSocketCount(Thread* thread);
    SEV_DECL static bool isSocketFull(Thread* thread);

public:
    SEV_DECL static SocketController* getController(Thread* thread);
    SEV_DECL static SocketController* getController();

private:
    Network() = delete;
    ~Network() = delete;
};

SEV_NS_END

#endif // SUBEVENT_NETWORK_HPP
