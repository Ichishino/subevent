#ifndef SUBEVENT_SOCKET_CONTROLLER_INL
#define SUBEVENT_SOCKET_CONTROLLER_INL

#include <cassert>
#include <utility>

#include <subevent/socket_controller.hpp>
#include <subevent/thread.hpp>
#include <subevent/timer.hpp>
#include <subevent/tcp.hpp>
#include <subevent/udp.hpp>

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
// SocketController
//---------------------------------------------------------------------------//

SocketController::SocketController()
{
}

SocketController::~SocketController()
{
}

WaitResult SocketController::wait(uint32_t msec, Event*& event)
{
    SocketSelector::SocketEvents sockEvents;

    WaitResult result = mSelector.wait(msec, sockEvents);

    switch (result)
    {
    case WaitResult::Success:
        onSelectEvent(sockEvents);
        break;
    case WaitResult::Cancel:
        {
            if (!sockEvents.isEmpty())
            {
                onSelectEvent(sockEvents);
            }

            event = pop();
            if (event != nullptr)
            {
                result = WaitResult::Success;
            }
        }
        break;
    case WaitResult::Timeout:
        break;
    case WaitResult::Error:
        break;
    }

    return result;
}

void SocketController::wakeup()
{
    mSelector.cancel();
}

bool SocketController::onInit()
{
    int32_t errorCode = mSelector.getErrorCode();

    if (errorCode != 0)
    {
        return false;
    }

    return true;
}

void SocketController::onExit()
{
    closeAllItems();
    bool finished = isAllItemsClosed();

    while (finished == false)
    {
        Event* event = nullptr;
        static const uint32_t msec = 3000;

        // wait
        WaitResult result = wait(msec, event);

        switch (result)
        {
        case WaitResult::Success:
        case WaitResult::Cancel:
            {
                delete event;
                finished = isAllItemsClosed();
            }
            break;
        case WaitResult::Timeout:
            {
                for (auto& pair : mTcpChannels)
                {
                    TcpChannelItem& item = pair.second;

                    mSelector.unregisterSocket(item.key);

                    delete item.socket;
                    delete item.closeTimer;
                }
                mTcpChannels.clear();
                finished = true;
            }
            break;
        case WaitResult::Error:
            assert(false);
            finished = true;
            break;
        }
    }

    EventController::onExit();
}

void SocketController::closeAllItems()
{
    // TcpServer
    for (auto& pair : mTcpServers)
    {
        TcpServerItem& item = pair.second;

        mSelector.unregisterSocket(item.key);
    }
    mTcpServers.clear();

    // TcpClient
    for (auto& pair : mTcpClients)
    {
        TcpClientItem& item = pair.second;

        mSelector.unregisterSocket(item.key);

        delete item.socket;
        delete item.cancelTimer;
    }
    mTcpClients.clear();

    // TcpChannel
    for (auto& pair : mTcpChannels)
    {
        TcpChannelItem& item = pair.second;

        if (item.tcpChannel != nullptr)
        {
            item.tcpChannel->mSocket->shutdown(
                Socket::ShutdownSend);

            item.socket = item.tcpChannel->mSocket;
            item.tcpChannel->mSocket = nullptr;
            item.tcpChannel = nullptr;
            item.sendBuffer.clear();
        }
    }

    // UdpReceiver
    for (auto& pair : mUdpReceivers)
    {
        UdpReceiverItem& item = pair.second;

        mSelector.unregisterSocket(item.key);
    }
    mUdpReceivers.clear();
}

bool SocketController::tryTcpConnect(TcpClientItem& item)
{
    while (!item.endPointList.empty())
    {
        IpEndPoint& peerEndPoint = item.endPointList.front();

        int32_t errorCode;
        Socket* socket =
            item.tcpClient->createSocket(peerEndPoint, errorCode);
        if (socket == nullptr)
        {
            item.tcpClient->onConnect(nullptr, errorCode);
            return true;
        }

        Socket::Handle sockHandle = socket->getHandle();

        if (!mSelector.registerSocket(
            sockHandle, SocketSelector::Connect, item.key))
        {
            item.tcpClient->onConnect(nullptr, -5103);
            delete socket;
            return true;
        }

        // connect
        bool result = socket->connect(peerEndPoint);

        item.endPointList.pop_front();

        if (result)
        {
            // success
            item.socket = socket;
            mTcpClients[sockHandle] = item;
            onSelectTcpConnect(sockHandle, 0);

            return false;
        }
        else if (socket->isBlockingError())
        {
            // blocking
            item.socket = socket;
            mTcpClients[sockHandle] = item;

            return false;
        }
        else
        {
            // error
            item.lastErrorCode = socket->getErrorCode();

            mSelector.unregisterSocket(item.key);
            delete socket;
        }
    }

    item.tcpClient->onConnect(nullptr, item.lastErrorCode);

    return true;
}

void SocketController::tryTcpSend(TcpChannelItem& item)
{
    while (!item.sendBuffer.empty())
    {
        TcpChannelItem::SendData& sendData =
            item.sendBuffer.front();

        size_t size = sendData.buff.size() - sendData.index;
        Socket* socket = item.tcpChannel->mSocket;

        // send
        int32_t result = socket->send(
            &sendData.buff[sendData.index],
            static_cast<uint32_t>(size), Socket::SendFlags);

        if (result >= 0)
        {
            sendData.index += static_cast<size_t>(result);

            if (sendData.index == sendData.buff.size())
            {
                // success
                item.tcpChannel->onSend(0);
            }
            else
            {
                break;
            }
        }
        else
        {
            if (socket->isBlockingError())
            {
                // blocking
                item.sendBlocked = true;
                break;
            }
            else
            {
                // error
                item.tcpChannel->onSend(
                    socket->getErrorCode());
            }
        }

        item.sendBuffer.pop_front();
    }
}

void SocketController::startTcpChannelCloseTimer(TcpChannelItem& item)
{
    Socket::Handle sockHandle =
        item.tcpChannel->mSocket->getHandle();

    static const uint32_t msec = 3000;

    item.closeTimer = new Timer();
    item.closeTimer->start(msec, false, [this, sockHandle](Timer*) {

        auto it = mTcpChannels.find(sockHandle);
        if (it == mTcpChannels.end())
        {
            return;
        }

        TcpChannelItem& timeOutItem = it->second;

        mSelector.unregisterSocket(timeOutItem.key);

        delete timeOutItem.socket;
        delete timeOutItem.closeTimer;

        mTcpChannels.erase(it);
    });
}

//---------------------------------------------------------------------------//
// Select Event
//---------------------------------------------------------------------------//

void SocketController::onSelectEvent(SocketSelector::SocketEvents& sockEvents)
{
    for (auto& item : sockEvents.read)
    {
        // accept
        if (onSelectTcpAccept(item.sockHandle, item.errorCode))
        {
            continue;
        }

        // receive (TCP)
        if (onSelectTcpReceive(item.sockHandle, item.errorCode))
        {
            continue;
        }

        // receive (UDP)
        if (onSelectUdpReceive(item.sockHandle, item.errorCode))
        {
            continue;
        }
    }

    for (auto& item : sockEvents.write)
    {
        // connect
        if (onSelectTcpConnect(item.sockHandle, item.errorCode))
        {
            continue;
        }

        // send
        if (onSelectTcpSend(item.sockHandle, item.errorCode))
        {
            continue;
        }
    }

    for (auto& item : sockEvents.close)
    {
        // close
        if (onSelectTcpClose(item.sockHandle, item.errorCode))
        {
            continue;
        }
    }
}

bool SocketController::onSelectTcpAccept(
    Socket::Handle sockHandle, int32_t /* errorCode */)
{
    auto it = mTcpServers.find(sockHandle);
    if (it == mTcpServers.end())
    {
        return false;
    }

    it->second.tcpServer->onAccept();

    return true;
}

bool SocketController::onSelectTcpConnect(
    Socket::Handle sockHandle, int32_t errorCode)
{
    auto it = mTcpClients.find(sockHandle);
    if (it == mTcpClients.end())
    {
        return false;
    }

    TcpClientItem item = it->second;

    mTcpClients.erase(it);
    mSelector.unregisterSocket(item.key);

    if (errorCode == 0)
    {
        // success

        delete item.cancelTimer;
        item.cancelTimer = nullptr;

        item.tcpClient->onConnect(item.socket, 0);
    }
    else
    {
        item.lastErrorCode = errorCode;

        // error
        delete item.socket;
        item.socket = nullptr;

        // next
        if (tryTcpConnect(item))
        {
            delete item.cancelTimer;
            item.cancelTimer = nullptr;
        }
    }

    return true;
}

bool SocketController::onSelectTcpReceive(
    Socket::Handle sockHandle, int32_t /* errorCode */)
{
    auto it = mTcpChannels.find(sockHandle);
    if (it == mTcpChannels.end())
    {
        return false;
    }

    TcpChannelItem& item = it->second;

    if (item.tcpChannel != nullptr)
    {
        item.tcpChannel->onReceive();
    }

    return true;
}

bool SocketController::onSelectTcpSend(
    Socket::Handle sockHandle, int32_t /* errorCode */)
{
    auto it = mTcpChannels.find(sockHandle);
    if (it == mTcpChannels.end())
    {
        return false;
    }

    TcpChannelItem& item = it->second;
    item.sendBlocked = false;

    tryTcpSend(item);

    return true;
}

bool SocketController::onSelectTcpClose(
    Socket::Handle sockHandle, int32_t /* errorCode */)
{
    auto it = mTcpChannels.find(sockHandle);
    if (it == mTcpChannels.end())
    {
        return false;
    }

    TcpChannelItem& item = it->second;
    TcpChannelPtr tcpChannel = item.tcpChannel;

    while (!item.sendBuffer.empty())
    {
        if (tcpChannel != nullptr)
        {
            tcpChannel->onSend(-5211);
        }

        item.sendBuffer.pop_front();
    }

    if (tcpChannel != nullptr)
    {
        char eof[1];
        int32_t result =
            tcpChannel->mSocket->receive(eof, sizeof(eof), MSG_PEEK);
        if (result <= 0)
        {
            mSelector.unregisterSocket(item.key);

            tcpChannel->onClose();
        }
        else
        {
            // close later
            return true;
        }
    }
    else
    {
        mSelector.unregisterSocket(item.key);

        delete item.socket;
        delete item.closeTimer;
    }

    mTcpChannels.erase(it);

    return true;
}

bool SocketController::onSelectUdpReceive(
    Socket::Handle sockHandle, int32_t /* errorCode */)
{
    auto it = mUdpReceivers.find(sockHandle);
    if (it == mUdpReceivers.end())
    {
        return false;
    }

    UdpReceiverItem& item = it->second;

    item.udpReceiver->onReceive();

    return true;
}

//---------------------------------------------------------------------------//
// Register / Unregister (TCP)
//---------------------------------------------------------------------------//

bool SocketController::registerTcpServer(const TcpServerPtr& tcpServer)
{
    Socket::Handle sockHandle =
        tcpServer->mSocket->getHandle();

    auto it = mTcpServers.find(sockHandle);
    if (it != mTcpServers.end())
    {
        return false;
    }

    TcpServerItem& item = mTcpServers[sockHandle];
    item.tcpServer = tcpServer;

    if (!mSelector.registerSocket(
        sockHandle, SocketSelector::Accept, item.key))
    {
        mTcpServers.erase(sockHandle);
        return false;
    }

    return true;
}

void SocketController::unregisterTcpServer(const TcpServerPtr& tcpServer)
{
    Socket::Handle sockHandle =
        tcpServer->mSocket->getHandle();

    auto it = mTcpServers.find(sockHandle);
    if (it == mTcpServers.end())
    {
        return;
    }

    mSelector.unregisterSocket(it->second.key);
    mTcpServers.erase(it);
}

bool SocketController::registerTcpChannel(const TcpChannelPtr& tcpChannel)
{
    Socket::Handle sockHandle =
        tcpChannel->mSocket->getHandle();

    auto it = mTcpChannels.find(sockHandle);
    if (it != mTcpChannels.end())
    {
        return false;
    }

    TcpChannelItem& item = mTcpChannels[sockHandle];
    item.tcpChannel = tcpChannel;
    item.socket = nullptr;
    item.closeTimer = nullptr;
    item.sendBlocked = true;

    if (!mSelector.registerSocket(sockHandle,
        (SocketSelector::Close |
         SocketSelector::Receive |
         SocketSelector::Send),
        item.key))
    {
        mTcpChannels.erase(sockHandle);
        return false;
    }

    return true;
}

void SocketController::unregisterTcpChannel(const TcpChannelPtr& tcpChannel)
{
    Socket::Handle sockHandle =
        tcpChannel->mSocket->getHandle();

    auto it = mTcpChannels.find(sockHandle);
    if (it == mTcpChannels.end())
    {
        return;
    }

    TcpChannelItem& item = it->second;

    mSelector.unregisterSocket(item.key);
    mTcpChannels.erase(it);
}

//---------------------------------------------------------------------------//
// Request / Cancel (TCP)
//---------------------------------------------------------------------------//

void SocketController::requestTcpConnect(
    const TcpClientPtr& tcpClient,
    const std::list<IpEndPoint>& endPointList,
    uint32_t msecTimeout)
{
    TcpClientItem item;
    item.tcpClient = tcpClient;
    item.endPointList = endPointList;
    item.msecTimeout = msecTimeout;
    item.lastErrorCode = -5100;

    item.cancelTimer = new Timer();
    item.cancelTimer->start(
        msecTimeout, false, [this, tcpClient](Timer*) {

        if (cancelTcpConnect(tcpClient))
        {
            tcpClient->onConnect(nullptr, -5102);
        }
    });

    if (tryTcpConnect(item))
    {
        delete item.cancelTimer;
    }
}

bool SocketController::cancelTcpConnect(const TcpClientPtr& tcpClient)
{
    for (auto& pair : mTcpClients)
    {
        TcpClientItem& item = pair.second;

        if (item.tcpClient == tcpClient)
        {
            mSelector.unregisterSocket(item.key);

            delete item.socket;
            delete item.cancelTimer;

            mTcpClients.erase(pair.first);

            return true;
        }
    }

    return false;
}

bool SocketController::requestTcpSend(
    const TcpChannelPtr& tcpChannel,
    std::vector<char>&& data)
{
    Socket::Handle sockHandle =
        tcpChannel->mSocket->getHandle();

    auto it = mTcpChannels.find(sockHandle);
    if (it == mTcpChannels.end())
    {
        return false;
    }

    TcpChannelItem& item = it->second;

    TcpChannelItem::SendData sendData;
    sendData.buff = std::move(data);
    sendData.index = 0;
    item.sendBuffer.push_back(std::move(sendData));

    if (!item.sendBlocked)
    {
        tryTcpSend(item);
    }

    return true;
}

bool SocketController::cancelTcpSend(const TcpChannelPtr& tcpChannel)
{
    Socket::Handle sockHandle =
        tcpChannel->mSocket->getHandle();

    auto it = mTcpChannels.find(sockHandle);
    if (it == mTcpChannels.end())
    {
        return false;
    }

    TcpChannelItem& item = it->second;

    if (item.sendBuffer.empty())
    {
        return false;
    }

    item.sendBuffer.clear();

    return true;
}

void SocketController::requestTcpChannelClose(const TcpChannelPtr& tcpChannel)
{
    Socket::Handle sockHandle =
        tcpChannel->mSocket->getHandle();

    auto it = mTcpChannels.find(sockHandle);
    if (it == mTcpChannels.end())
    {
        return;
    }

    TcpChannelItem& item = it->second;

    // shutdown
    item.tcpChannel->mSocket->shutdown(Socket::ShutdownSend);
    startTcpChannelCloseTimer(item);

    item.socket = item.tcpChannel->mSocket;
    item.tcpChannel->mSocket = nullptr;
    item.tcpChannel = nullptr;
    item.sendBuffer.clear();
}

void SocketController::onTcpReceiveEof(const TcpChannelPtr& tcpChannel)
{
    Socket::Handle sockHandle =
        tcpChannel->mSocket->getHandle();

    auto it = mTcpChannels.find(sockHandle);
    if (it == mTcpChannels.end())
    {
        return;
    }

    TcpChannelItem& item = it->second;

    mSelector.unregisterSocket(item.key);

    item.tcpChannel->onClose();

    mTcpChannels.erase(it);
}

//---------------------------------------------------------------------------//
// Register / Unregister (UDP)
//---------------------------------------------------------------------------//

bool SocketController::registerUdpReceiver(const UdpReceiverPtr& udpReceiver)
{
    Socket::Handle sockHandle =
        udpReceiver->mSocket->getHandle();

    auto it = mUdpReceivers.find(sockHandle);
    if (it != mUdpReceivers.end())
    {
        return false;
    }

    UdpReceiverItem& item = mUdpReceivers[sockHandle];
    item.udpReceiver = udpReceiver;

    if (!mSelector.registerSocket(
        sockHandle, SocketSelector::Receive, item.key))
    {
        mUdpReceivers.erase(sockHandle);
        return false;
    }

    return true;
}

void SocketController::unregisterUdpReceiver(const UdpReceiverPtr& udpReceiver)
{
    Socket::Handle sockHandle =
        udpReceiver->mSocket->getHandle();

    auto it = mUdpReceivers.find(sockHandle);
    if (it == mUdpReceivers.end())
    {
        return;
    }

    UdpReceiverItem& item = it->second;

    mSelector.unregisterSocket(item.key);
    mUdpReceivers.erase(it);
}

SEV_NS_END

#endif // SUBEVENT_SOCKET_CONTROLLER_INL
