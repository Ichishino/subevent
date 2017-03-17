#ifndef SUBEVENT_SOCKET_CONTROLLER_HPP
#define SUBEVENT_SOCKET_CONTROLLER_HPP

#include <map>
#include <list>

#include <subevent/std.hpp>
#include <subevent/event_controller.hpp>
#include <subevent/socket_selector.hpp>

SEV_NS_BEGIN

class Event;
class Timer;
class Socket;
class TcpServer;
class TcpClient;
class TcpChannel;
class UdpReceiver;
class UdpSender;

//---------------------------------------------------------------------------//
// SocketController
//---------------------------------------------------------------------------//

class SocketController : public EventController
{
public:
    SEV_DECL SocketController();
    SEV_DECL ~SocketController();

public:
    SEV_DECL WaitResult wait(uint32_t msec, Event*& event) override;
    SEV_DECL void wakeup() override;
    SEV_DECL void onExit() override;

public:
    SEV_DECL bool registerTcpServer(TcpServer* tcpServer);
    SEV_DECL void unregisterTcpServer(TcpServer* tcpServer);

    SEV_DECL bool registerTcpChannel(TcpChannel* tcpChannel);
    SEV_DECL void unregisterTcpChannel(TcpChannel* tcpChannel);

    SEV_DECL void requestTcpConnect(
        TcpClient* tcpClient,
        const std::list<IpEndPoint>& endPointList,
        uint32_t msecTimeout);
    SEV_DECL bool cancelTcpConnect(TcpClient* tcpClient);

    SEV_DECL bool requestTcpSend(
        TcpChannel* tcpChannel, const void* data, uint32_t size);
    SEV_DECL bool cancelTcpSend(TcpChannel* tcpChannel);

    SEV_DECL void requestTcpChannelClose(TcpChannel* tcpChannel);

    SEV_DECL bool registerUdpReceiver(UdpReceiver* udpReceiver);
    SEV_DECL void unregisterUdpReceiver(UdpReceiver* udpReceiver);

    SEV_DECL void onTcpReceiveEof(TcpChannel* tcpChannel);

public:
    SEV_DECL uint32_t getSocketCount() const
    {
        return mSelector.getSocketCount();
    }

    SEV_DECL bool isFull() const
    {
        return (getSocketCount() >= SocketSelector::MaxSockets);
    }

private:
    SEV_DECL void onSelectEvent(SocketSelector::SocketEvents& sockEvents);
    SEV_DECL bool onSelectTcpAccept(Socket::Handle sockHandle, int32_t errorCode);
    SEV_DECL bool onSelectTcpConnect(Socket::Handle sockHandle, int32_t errorCode);
    SEV_DECL bool onSelectTcpReceive(Socket::Handle sockHandle, int32_t errorCode);
    SEV_DECL bool onSelectTcpSend(Socket::Handle sockHandle, int32_t errorCode);
    SEV_DECL bool onSelectTcpClose(Socket::Handle sockHandle, int32_t errorCode);
    SEV_DECL bool onSelectUdpReceive(Socket::Handle sockHandle, int32_t errorCode);

    SEV_DECL void closeAllItems();

    SEV_DECL bool isAllItemsClosed() const
    {
        return mTcpServers.empty() &&
            mTcpClients.empty() &&
            mTcpChannels.empty() &&
            mUdpReceivers.empty();
    }

    SocketSelector mSelector;

    struct TcpServerItem
    {
        SocketSelector::RegKey key;
        TcpServer* tcpServer;
    };

    struct TcpClientItem
    {
        SocketSelector::RegKey key;
        TcpClient* tcpClient;
        Socket* socket;

        std::list<IpEndPoint> endPointList;
        uint32_t msecTimeout;
        Timer* cancelTimer;

        int32_t lastErrorCode;
    };

    struct TcpChannelItem
    {
        SocketSelector::RegKey key;
        TcpChannel* tcpChannel;
        Socket* socket;

        bool sendBlocked;

        struct SendData
        {
            const char* addr;
            uint32_t size;
            uint32_t index;
        };

        std::list<SendData> sendBuffer;
        Timer* closeTimer;
    };

    struct UdpReceiverItem
    {
        SocketSelector::RegKey key;
        UdpReceiver* udpReceiver;

    };

    SEV_DECL bool tryTcpConnect(TcpClientItem& item);
    SEV_DECL void tryTcpSend(TcpChannelItem& item);
    SEV_DECL void startTcpChannelCloseTimer(TcpChannelItem& item);

    std::map<Socket::Handle, TcpServerItem> mTcpServers;
    std::map<Socket::Handle, TcpClientItem> mTcpClients;
    std::map<Socket::Handle, TcpChannelItem> mTcpChannels;
    std::map<Socket::Handle, UdpReceiverItem> mUdpReceivers;
};

SEV_NS_END

#endif // SUBEVENT_SOCKET_CONTROLLER_HPP
