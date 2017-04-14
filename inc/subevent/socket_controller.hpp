#ifndef SUBEVENT_SOCKET_CONTROLLER_HPP
#define SUBEVENT_SOCKET_CONTROLLER_HPP

#include <map>
#include <list>
#include <vector>

#include <subevent/std.hpp>
#include <subevent/event_controller.hpp>
#include <subevent/socket_selector.hpp>
#include <subevent/tcp.hpp>
#include <subevent/udp.hpp>

SEV_NS_BEGIN

class Event;
class Timer;

//---------------------------------------------------------------------------//
// SocketController
//---------------------------------------------------------------------------//

class SocketController : public EventController
{
public:
    SEV_DECL SocketController();
    SEV_DECL ~SocketController() override;

public:
    SEV_DECL WaitResult wait(uint32_t msec, Event*& event) override;
    SEV_DECL void wakeup() override;
    SEV_DECL bool onInit() override;
    SEV_DECL void onExit() override;

public:
    SEV_DECL bool registerTcpServer(const TcpServerPtr& tcpServer);
    SEV_DECL void unregisterTcpServer(const TcpServerPtr& tcpServer);

    SEV_DECL bool registerTcpChannel(const TcpChannelPtr& tcpChannel);
    SEV_DECL void unregisterTcpChannel(const TcpChannelPtr& tcpChannel);

    SEV_DECL void requestTcpConnect(
        const TcpClientPtr& tcpClient,
        const std::list<IpEndPoint>& endPointList,
        uint32_t msecTimeout);
    SEV_DECL bool cancelTcpConnect(const TcpClientPtr& tcpClient);

    SEV_DECL bool requestTcpSend(
        const TcpChannelPtr& tcpChannel,
        std::vector<char>&& data);
    SEV_DECL bool cancelTcpSend(const TcpChannelPtr& tcpChannel);

    SEV_DECL void requestTcpChannelClose(const TcpChannelPtr& tcpChannel);

    SEV_DECL bool registerUdpReceiver(const UdpReceiverPtr& udpReceiver);
    SEV_DECL void unregisterUdpReceiver(const UdpReceiverPtr& udpReceiver);

    SEV_DECL void onTcpReceiveEof(const TcpChannelPtr& tcpChannel);

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
        TcpServerPtr tcpServer;
    };

    struct TcpClientItem
    {
        SocketSelector::RegKey key;
        TcpClientPtr tcpClient;
        Socket* socket;

        std::list<IpEndPoint> endPointList;
        uint32_t msecTimeout;
        Timer* cancelTimer;

        int32_t lastErrorCode;
    };

    struct TcpChannelItem
    {
        SocketSelector::RegKey key;
        TcpChannelPtr tcpChannel;
        Socket* socket;

        bool sendBlocked;

        struct SendData
        {
            std::vector<char> buff;
            size_t index;
        };

        std::list<SendData> sendBuffer;
        Timer* closeTimer;
    };

    struct UdpReceiverItem
    {
        SocketSelector::RegKey key;
        UdpReceiverPtr udpReceiver;

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
