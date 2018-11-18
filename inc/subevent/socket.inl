#ifndef SUBEVENT_SOCKET_INL
#define SUBEVENT_SOCKET_INL

#include <cstring>

#include <subevent/socket.hpp>

#ifdef SEV_OS_WIN
#else
#include <unistd.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#endif

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
// SocketOption
//---------------------------------------------------------------------------//

SocketOption::SocketOption()
{
    mSocket = nullptr;
    mStore = new Map();
}

SocketOption::SocketOption(Socket* socket)
{
    mSocket = socket;
    mStore = nullptr;
}

SocketOption::SocketOption(const SocketOption& other)
{
    operator=(other);
}

SocketOption::SocketOption(SocketOption&& other)
{
    operator=(other);
}

SocketOption::~SocketOption()
{
    delete mStore;
}

SocketOption& SocketOption::operator=(const SocketOption& other)
{
    if (this != &other)
    {
        mSocket = other.mSocket;

        if (other.mStore != nullptr)
        {
            if (mStore == nullptr)
            {
                mStore = new Map();
            }

            *mStore = *other.mStore;
        }
        else
        {
            delete mStore;
            mStore = nullptr;
        }
    }

    return *this;
}

SocketOption& SocketOption::operator=(SocketOption&& other)
{
    if (this != &other)
    {
        mSocket = other.mSocket;

        delete mStore;
        mStore = other.mStore;

        other.mSocket = nullptr;
        other.mStore = nullptr;
    }

    return *this;
}

void SocketOption::setOption(
    int32_t level, int32_t name, const void* value, socklen_t size)
{
    if (mSocket != nullptr)
    {
        mSocket->setOption(level, name, value, size);
    }
    else
    {
        auto& storeValue = (*mStore)[Key(level, name)];
        storeValue.resize(size);
        memcpy(&storeValue[0], value, size);
    }
}

bool SocketOption::getOption(
    int32_t level, int32_t name, void* value, socklen_t* size) const
{
    if (mSocket != nullptr)
    {
        return mSocket->getOption(level, name, value, size);
    }
    else
    {
        auto it = mStore->find(Key(level, name));

        if (it == mStore->end())
        {
            return false;
        }
        else
        {
            memcpy(value, &it->second[0], *size);
            return true;
        }
    }
}

void SocketOption::clear()
{
    mSocket = nullptr;

    if (mStore == nullptr)
    {
        mStore = new Map();
    }
    else
    {
        mStore->clear();
    }
}

void SocketOption::setReuseAddress(bool on)
{
    int32_t value = (on ? 1 : 0);

    setOption(SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
}

bool SocketOption::getReuseAddress(bool& on) const
{
    int32_t value;
    socklen_t size = sizeof(value);

    if (!getOption(SOL_SOCKET, SO_REUSEADDR, &value, &size))
    {
        return false;
    }

    on = (value == 0 ? false : true);

    return true;
}

void SocketOption::setKeepAlive(bool on)
{
    int32_t value = (on ? 1 : 0);

    setOption(SOL_SOCKET, SO_KEEPALIVE, &value, sizeof(value));
}

bool SocketOption::getKeepAlive(bool& on) const
{
    int32_t value;
    socklen_t size = sizeof(value);

    if (!getOption(SOL_SOCKET, SO_KEEPALIVE, &value, &size))
    {
        return false;
    }

    on = (value == 0 ? false : true);

    return true;
}

void SocketOption::setLinger(bool on, uint16_t sec)
{
    struct linger value;
    value.l_onoff = (on ? 1 : 0);
    value.l_linger = sec;

    setOption(SOL_SOCKET, SO_LINGER, &value, sizeof(value));
}

bool SocketOption::getLinger(bool& on, uint16_t& sec) const
{
    struct linger value;
    socklen_t size = sizeof(value);

    if (!getOption(SOL_SOCKET, SO_LINGER, &value, &size))
    {
        return false;
    }

    on = (value.l_onoff == 0 ? false : true);
    sec = value.l_linger;

    return true;
}

void SocketOption::setReceiveBuffSize(uint32_t buffSize)
{
    setOption(SOL_SOCKET, SO_RCVBUF, &buffSize, sizeof(buffSize));
}

bool SocketOption::getReceiveBuffSize(uint32_t& buffSize) const
{
    socklen_t size = sizeof(buffSize);

    if (!getOption(SOL_SOCKET, SO_RCVBUF, &buffSize, &size))
    {
        return false;
    }

    return true;
}

void SocketOption::setSendBuffSize(uint32_t size)
{
    setOption(SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
}

bool SocketOption::getSendBuffSize(uint32_t& buffSize) const
{
    socklen_t size = sizeof(buffSize);

    if (!getOption(SOL_SOCKET, SO_SNDBUF, &buffSize, &size))
    {
        return false;
    }

    return true;
}

void SocketOption::setIpv6Only(bool on)
{
    int32_t value = (on ? 1 : 0);

    setOption(IPPROTO_IPV6, IPV6_V6ONLY, &value, sizeof(value));
}

bool SocketOption::getIpv6Only(bool& on) const
{
    int32_t value;
    socklen_t size = sizeof(value);

    if (!getOption(IPPROTO_IPV6, IPV6_V6ONLY, &value, &size))
    {
        return false;
    }

    on = (value == 0 ? false : true);

    return true;
}

void SocketOption::setTcpNoDelay(bool on)
{
    int32_t value = (on ? 1 : 0);

    setOption(IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
}

bool SocketOption::getTcpNoDelay(bool& on) const
{
    int32_t value;
    socklen_t size = sizeof(value);

    if (!getOption(IPPROTO_TCP, TCP_NODELAY, &value, &size))
    {
        return false;
    }

    on = (value == 0 ? false : true);

    return true;
}

void SocketOption::setBroadcast(bool on)
{
    int32_t value = (on ? 1 : 0);

    setOption(SOL_SOCKET, SO_BROADCAST, &value, sizeof(value));
}

bool SocketOption::getBroadcast(bool& on) const
{
    int32_t value;
    socklen_t size = sizeof(value);

    if (!getOption(SOL_SOCKET, SO_BROADCAST, &value, &size))
    {
        return false;
    }

    on = (value == 0 ? false : true);

    return true;
}

//---------------------------------------------------------------------------//
// IpEndPoint
//---------------------------------------------------------------------------//

IpEndPoint::IpEndPoint()
{
    memset(&mSockAddr, 0x00, sizeof(mSockAddr));
    mSockAddr.any.ss_family = AF_UNSPEC;
}

IpEndPoint::IpEndPoint(const std::string& address, uint16_t port)
{
    memset(&mSockAddr, 0x00, sizeof(mSockAddr));

    setAddress(address);
    setPort(port);
}

IpEndPoint::IpEndPoint(uint16_t port, const AddressFamily& family)
{
    memset(&mSockAddr, 0x00, sizeof(mSockAddr));

    mSockAddr.any.ss_family = static_cast<uint16_t>(family);
    setPort(port);
}

IpEndPoint::IpEndPoint(const struct sockaddr* sock_addr, uint32_t size)
{
    memcpy(&mSockAddr, sock_addr, size);
}

IpEndPoint::IpEndPoint(const IpEndPoint& other)
{
    operator=(other);
}

IpEndPoint::~IpEndPoint()
{
}

IpEndPoint& IpEndPoint::operator=(const IpEndPoint& other)
{
    memcpy(&mSockAddr, other.getTable(), other.getTableSize());
    return *this;
}

AddressFamily IpEndPoint::getFamily() const
{
    if (mSockAddr.any.ss_family == AF_INET)
    {
        return AddressFamily::Ipv4;
    }
    else if (mSockAddr.any.ss_family == AF_INET6)
    {
        return AddressFamily::Ipv6;
    }

    return AddressFamily::Unspec;
}

std::string IpEndPoint::getAddress() const
{
    uint16_t family = AF_UNSPEC;
    const void* addr = nullptr;

    if (isIpv4())
    {
        family = AF_INET;
        addr = &mSockAddr.v4.sin_addr;
    }
    else if (isIpv6())
    {
        family = AF_INET6;
        addr = &mSockAddr.v6.sin6_addr;
    }
    else
    {
        return std::string();
    }

    char buff[512];

    if (inet_ntop(family, const_cast<void*>(addr),
        buff, sizeof(buff)) == nullptr)
    {
        return std::string();
    }

    return std::string(buff);
}

void IpEndPoint::setAddress(const std::string& address)
{
    if (inet_pton(AF_INET,
        address.c_str(), &mSockAddr.v4.sin_addr) == 1)
    {
        // Ipv4
        mSockAddr.v4.sin_family = AF_INET;

    }
    else if (inet_pton(AF_INET6,
        address.c_str(), &mSockAddr.v6.sin6_addr) == 1)
    {
        // Ipv6
        mSockAddr.v6.sin6_family = AF_INET6;
    }
}

uint16_t IpEndPoint::getPort() const
{
    if (isIpv4())
    {
        return ntohs(mSockAddr.v4.sin_port);
    }
    else if (isIpv6())
    {
        return ntohs(mSockAddr.v6.sin6_port);
    }

    return 0;
}

void IpEndPoint::setPort(uint16_t port)
{
    if (isIpv4())
    {
        mSockAddr.v4.sin_port = htons(port);
    }
    else if (isIpv6())
    {
        mSockAddr.v6.sin6_port = htons(port);
    }
}

std::string IpEndPoint::toString() const
{
    if (isIpv4())
    {
        return std::string(
            getAddress() + ":" + std::to_string(getPort()));
    }
    else if (isIpv6())
    {
        return std::string(
            getAddress() + "." + std::to_string(getPort()));
    }

    return std::string();
}

void IpEndPoint::clear()
{
    memset(&mSockAddr, 0x00, sizeof(mSockAddr));
}

uint32_t IpEndPoint::getTableSize() const
{
    if (isIpv4())
    {
        return sizeof(mSockAddr.v4);
    }
    else if (isIpv6())
    {
        return sizeof(mSockAddr.v6);
    }

    return sizeof(mSockAddr);
}

std::list<IpEndPoint> IpEndPoint::resolveName(
    const std::string& node, uint16_t port,
    const AddressFamily& family, const Socket::Type& type,
    uint32_t flags)
{
    return resolveService(
        node, std::to_string(port), family, type, (flags | AI_NUMERICSERV));
}

std::list<IpEndPoint> IpEndPoint::resolveService(
    const std::string& node, const std::string& service,
    const AddressFamily& family, const Socket::Type& type,
    uint32_t flags)
{
    addrinfo in;
    memset(&in, 0x00, sizeof(in));

    in.ai_family = static_cast<uint16_t>(family);
    in.ai_socktype = static_cast<uint16_t>(type);
    in.ai_flags = static_cast<int>(flags);

    const char* nodeParam = nullptr;
    if (!node.empty())
    {
        nodeParam = node.c_str();
    }
    else
    {
        flags |= AI_PASSIVE;
    }

    const char* serviceParam = nullptr;
    if (!service.empty())
    {
        serviceParam = service.c_str();
    }

    std::list<IpEndPoint> results;
    addrinfo* out = nullptr;

    if (getaddrinfo(nodeParam, serviceParam, &in, &out) != 0)
    {
        return results;
    }

    for (addrinfo* ai = out; ai != NULL; ai = ai->ai_next)
    {
        results.push_back(
            IpEndPoint(
                ai->ai_addr,
                static_cast<uint32_t>(ai->ai_addrlen)));
    }

    freeaddrinfo(out);

    return results;
}

//---------------------------------------------------------------------------//
// Socket
//---------------------------------------------------------------------------//

Socket::Socket(Handle handle)
{
    mHandle = handle;
    mErrorCode = 0;
}

Socket::~Socket()
{
    close();
}

bool Socket::create(
    const AddressFamily& family, const Type& type, const Protocol& protocol)
{
    mHandle = ::socket(
        static_cast<uint16_t>(family),
        static_cast<uint16_t>(type),
        static_cast<uint16_t>(protocol));

    mErrorCode = Socket::getLastError();

#ifdef SEV_OS_MAC
    if (mHandle != InvalidHandle)
    {
        int32_t value = 1;
        setOption(SOL_SOCKET, SO_NOSIGPIPE, &value, sizeof(value));
    }
#endif

    return (mHandle != InvalidHandle);
}

bool Socket::bind(const IpEndPoint& endPoint)
{
    int32_t result = ::bind(getHandle(),
        endPoint.getTable(), endPoint.getTableSize());

    mErrorCode = Socket::getLastError();

    return (result == 0);
}

void Socket::close()
{
    if (mHandle != InvalidHandle)
    {
#ifdef SEV_OS_WIN
        ::closesocket(mHandle);
#else
        ::close(mHandle);
#endif
        mHandle = InvalidHandle;
    }
}

void Socket::shutdown(int32_t how)
{
    ::shutdown(getHandle(), how);
}

bool Socket::listen(int32_t backlog)
{
    int32_t result = ::listen(getHandle(), backlog);

    mErrorCode = Socket::getLastError();

    return (result == 0);
}

Socket* Socket::accept()
{
    Socket* socket = nullptr;

    Handle handle = ::accept(
        getHandle(), nullptr, nullptr);
    if (handle != InvalidHandle)
    {
        socket = new Socket(handle);
    }

    mErrorCode = Socket::getLastError();

    return socket;
}

bool Socket::connect(const IpEndPoint& peerEndPoint)
{
    int32_t result = ::connect(getHandle(),
        peerEndPoint.getTable(), peerEndPoint.getTableSize());

    mErrorCode = Socket::getLastError();

    return (result == 0);
}

int32_t Socket::send(const void* data, uint32_t size, int32_t flags)
{
    int32_t result = static_cast<int32_t>(
        ::send(getHandle(), static_cast<const char*>(data), size, flags));

    mErrorCode = Socket::getLastError();

    return result;
}

int32_t Socket::receive(void* buff, uint32_t size, int32_t flags)
{
    int32_t result = static_cast<int32_t>(
        ::recv(getHandle(), static_cast<char*>(buff), size, flags));

    mErrorCode = Socket::getLastError();

    return static_cast<int32_t>(result);
}

int32_t Socket::sendTo(const IpEndPoint& receiverEndPoint,
    const void* data, uint32_t size, int32_t flags)
{
    int32_t result = static_cast<int32_t>(
        ::sendto(getHandle(),
            static_cast<const char*>(data), size, flags,
            receiverEndPoint.getTable(),
            receiverEndPoint.getTableSize()));

    mErrorCode = Socket::getLastError();

    return static_cast<int32_t>(result);
}

int32_t Socket::receiveFrom(IpEndPoint& senderEndPoint,
    void* buff, uint32_t size, int32_t flags)
{
    socklen_t tableSize = senderEndPoint.getTableSize();

    int32_t result = static_cast<int32_t>(
        ::recvfrom(getHandle(),
            static_cast<char*>(buff), size, flags,
            senderEndPoint.getTable(), &tableSize));

    mErrorCode = Socket::getLastError();

    return static_cast<int32_t>(result);
}

bool Socket::getLocalEndPoint(IpEndPoint& localEndPoint) const
{
    socklen_t size = localEndPoint.getTableSize();

    int result = ::getsockname(getHandle(),
        localEndPoint.getTable(), &size);

    mErrorCode = Socket::getLastError();

    return (result == 0);
}

bool Socket::getPeerEndPoint(IpEndPoint& peerEndPoint) const
{
    socklen_t size = peerEndPoint.getTableSize();

    int32_t result = ::getpeername(getHandle(),
        peerEndPoint.getTable(), &size);

    mErrorCode = Socket::getLastError();

    return (result == 0);
}

void Socket::setOption(const SocketOption& sockOption)
{
    for (const auto& option : *sockOption.mStore)
    {
        const SocketOption::Key& key = option.first;
        const SocketOption::Value& value = option.second;

        setOption(
            key.first, key.second,
            &value[0], static_cast<socklen_t>(value.size()));
    }
}

SocketOption Socket::getOption()
{
    return SocketOption(this);
}

bool Socket::setOption(int32_t level, int32_t name,
    const void* value, socklen_t size)
{
    int32_t result = ::setsockopt(getHandle(), level, name,
        static_cast<const char*>(value), size);

    mErrorCode = Socket::getLastError();

    return (result == 0);
}

bool Socket::getOption(int32_t level, int32_t name,
    void* buff, socklen_t* size) const
{
    int32_t result = ::getsockopt(getHandle(), level, name,
        static_cast<char*>(buff), size);

    mErrorCode = Socket::getLastError();

    return (result == 0);
}

bool Socket::isBlockingError() const
{
#ifdef SEV_OS_WIN
    return (getErrorCode() == WSAEWOULDBLOCK);
#else
    return ((getErrorCode() == EAGAIN) ||
            (getErrorCode() == EINPROGRESS) ||
            (getErrorCode() == EWOULDBLOCK));
#endif
}

bool Socket::onAccept()
{
    return true;
}

bool Socket::onConnect()
{
    return true;
}

int Socket::getLastError()
{
#ifdef SEV_OS_WIN
    return WSAGetLastError();
#else
    return errno;
#endif
}

SEV_NS_END

#endif // SUBEVENT_SOCKET_INL
