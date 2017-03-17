#ifndef SUBEVENT_SOCKET_INL
#define SUBEVENT_SOCKET_INL

#include <cstring>

#include <subevent/socket.hpp>

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
// SocketOption
//---------------------------------------------------------------------------//

SocketOption::SocketOption()
{
}

SocketOption::~SocketOption()
{
}

void SocketOption::clear()
{
    mOptions.clear();
}

void SocketOption::setOption(
    int32_t level, int32_t name, const void* value, uint32_t size)
{
    Option option;
    option.level = level;
    option.name = name;
    option.value.resize(size);
    memcpy(&option.value[0], value, size);

    mOptions.push_back(std::move(option));
}

void SocketOption::setReuseAddr(bool on)
{
    int32_t value = (on ? 1 : 0);

    setOption(SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
}

void SocketOption::setKeepAlive(bool on)
{
    int32_t value = (on ? 1 : 0);

    setOption(SOL_SOCKET, SO_KEEPALIVE, &value, sizeof(value));
}

void SocketOption::setLinger(bool on, uint16_t sec)
{
    struct linger value;
    value.l_onoff = (on ? 1 : 0);
    value.l_linger = sec;

    setOption(SOL_SOCKET, SO_LINGER, &value, sizeof(value));
}

void SocketOption::setReceiveBuffSize(uint32_t size)
{
    setOption(SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
}

void SocketOption::setSendBuffSize(uint32_t size)
{
    setOption(SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
}

void SocketOption::setIpv6Only(bool on)
{
    int32_t value = (on ? 1 : 0);

    setOption(IPPROTO_IPV6, IPV6_V6ONLY, &value, sizeof(value));
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

Socket::Socket()
{
    mHandle = InvalidHandle;
    mErrorCode = 0;
}

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
#ifdef _WIN32
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
    for (const auto& option : sockOption.getOptions())
    {
        setOption(
            option.level,
            option.name,
            &option.value[0],
            static_cast<socklen_t>(option.value.size()));
    }
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
#ifdef _WIN32
    return (getErrorCode() == WSAEWOULDBLOCK);
#else
    return ((getErrorCode() == EAGAIN) ||
            (getErrorCode() == EINPROGRESS) ||
            (getErrorCode() == EWOULDBLOCK));
#endif
}

int Socket::getLastError()
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

//---------------------------------------------------------------------------//
// WinSock
//---------------------------------------------------------------------------//

#ifdef _WIN32

#pragma comment(lib, "ws2_32.lib")

WinSock::WinSock()
{
    WORD sockVer = MAKEWORD(2, 2);
    WSADATA wsaData;

    mErrorCode = WSAStartup(sockVer, &wsaData);
}

WinSock::~WinSock()
{
    if (mErrorCode == 0)
    {
        WSACleanup();
    }
}

bool WinSock::init()
{
    static WinSock winSock;
    return (winSock.mErrorCode == 0);
}

#endif

SEV_NS_END

#endif // SUBEVENT_SOCKET_INL
