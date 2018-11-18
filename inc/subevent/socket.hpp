#ifndef SUBEVENT_SOCKET_HPP
#define SUBEVENT_SOCKET_HPP

#include <map>
#include <list>
#include <vector>
#include <string>
#include <atomic>

#include <subevent/std.hpp>

#ifdef SEV_OS_WIN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

SEV_NS_BEGIN

class Socket;
class IpEndPoint;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

enum class AddressFamily : uint16_t
{
    Unspec = AF_UNSPEC,
    Ipv4 = AF_INET,
    Ipv6 = AF_INET6
};

//---------------------------------------------------------------------------//
// SocketOption
//---------------------------------------------------------------------------//

class SocketOption
{
public:
    SEV_DECL SocketOption();
    SEV_DECL SocketOption(const SocketOption& other);
    SEV_DECL SocketOption(SocketOption&& other);
    SEV_DECL ~SocketOption();

public:
    SEV_DECL void setReuseAddress(bool on);
    SEV_DECL void setKeepAlive(bool on);
    SEV_DECL void setLinger(bool on, uint16_t sec);
    SEV_DECL void setReceiveBuffSize(uint32_t buffSize);
    SEV_DECL void setSendBuffSize(uint32_t buffSize);
    SEV_DECL void setIpv6Only(bool on);
    SEV_DECL void setTcpNoDelay(bool on);
    SEV_DECL void setBroadcast(bool on);

    SEV_DECL bool getReuseAddress(bool& on) const;
    SEV_DECL bool getKeepAlive(bool& on) const;
    SEV_DECL bool getLinger(bool& on, uint16_t& sec) const;
    SEV_DECL bool getReceiveBuffSize(uint32_t& buffSize) const;
    SEV_DECL bool getSendBuffSize(uint32_t& buffSize) const;
    SEV_DECL bool getIpv6Only(bool& on) const;
    SEV_DECL bool getTcpNoDelay(bool& on) const;
    SEV_DECL bool getBroadcast(bool& on) const;

public:
    SEV_DECL void setOption(
        int32_t level, int32_t name, const void* value, socklen_t size);
    SEV_DECL bool getOption(
        int32_t level, int32_t name, void* value, socklen_t* size) const;

    SEV_DECL void clear();

public:
    SEV_DECL SocketOption& operator=(const SocketOption& other);
    SEV_DECL SocketOption& operator=(SocketOption&& other);

private:
    SEV_DECL SocketOption(Socket* socket);

    Socket* mSocket;

    typedef std::pair<int32_t, int32_t> Key;
    typedef std::vector<char> Value;
    typedef std::map<Key, Value> Map;

    Map* mStore;

    friend class Socket;
};

//---------------------------------------------------------------------------//
// Socket
//---------------------------------------------------------------------------//

class Socket
{
public:

#ifdef SEV_OS_WIN
    typedef SOCKET Handle;
    static const Handle InvalidHandle = INVALID_SOCKET;
    static const int32_t ShutdownSend = SD_SEND;
    static const int32_t ShutdownBoth = SD_BOTH;
    static const int32_t SendFlags = 0;
#elif defined(SEV_OS_MAC)
    typedef int32_t Handle;
    static const Handle InvalidHandle = -1;
    static const int32_t ShutdownSend = SHUT_WR;
    static const int32_t ShutdownBoth = SHUT_RDWR;
    static const int32_t SendFlags = 0;
#elif defined(SEV_OS_LINUX)
    typedef int32_t Handle;
    static const Handle InvalidHandle = -1;
    static const int32_t ShutdownSend = SHUT_WR;
    static const int32_t ShutdownBoth = SHUT_RDWR;
    static const int32_t SendFlags = MSG_NOSIGNAL;
#endif

    SEV_DECL Socket(Handle handle = InvalidHandle);
    SEV_DECL virtual ~Socket();

    enum class Type : uint16_t
    {
        Tcp = SOCK_STREAM,
        Udp = SOCK_DGRAM
    };

    enum class Protocol : uint16_t
    {
        Tcp = IPPROTO_TCP,
        Udp = IPPROTO_UDP
    };

public:
    SEV_DECL bool create(
        const AddressFamily& family,
        const Type& type,
        const Protocol& protocol);
    SEV_DECL bool bind(const IpEndPoint& endPoint);

    SEV_DECL bool listen(int32_t backlog);
    SEV_DECL bool connect(const IpEndPoint& peerEndPoint);

    SEV_DECL void shutdown(int32_t how);

    SEV_DECL int32_t sendTo(const IpEndPoint& receiverEndPoint,
        const void* data, uint32_t size, int32_t flags = 0);
    SEV_DECL int32_t receiveFrom(IpEndPoint& senderEndPoint,
        void* buff, uint32_t size, int32_t flags = 0);

    SEV_DECL virtual Socket* accept();
    SEV_DECL virtual int32_t send(
        const void* data, uint32_t size, int32_t flags = 0);
    SEV_DECL virtual int32_t receive(
        void* buff, uint32_t size, int32_t flags = 0);
    SEV_DECL virtual void close();

public:
    SEV_DECL bool getLocalEndPoint(IpEndPoint& localEndPoint) const;
    SEV_DECL bool getPeerEndPoint(IpEndPoint& peerEndPoint) const;

    SEV_DECL void setOption(const SocketOption& sockOption);
    SEV_DECL SocketOption getOption();

    SEV_DECL bool setOption(int32_t level, int32_t name,
        const void* value, socklen_t size);
    SEV_DECL bool getOption(int32_t level, int32_t name,
        void* buff, socklen_t* size) const;

    SEV_DECL Handle getHandle() const
    {
        return mHandle;
    }

    SEV_DECL int getErrorCode() const
    {
        return mErrorCode;
    }

    SEV_DECL bool isBlockingError() const;

public:
    SEV_DECL virtual bool onAccept();
    SEV_DECL virtual bool onConnect();
    SEV_DECL static int getLastError();

protected:
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Handle mHandle;
    mutable int mErrorCode;
};

//---------------------------------------------------------------------------//
// IpEndPoint
//---------------------------------------------------------------------------//

class IpEndPoint
{
public:
    SEV_DECL IpEndPoint();
    SEV_DECL IpEndPoint(const std::string& address, uint16_t port);
    SEV_DECL IpEndPoint(
        uint16_t port, const AddressFamily& family = AddressFamily::Ipv4);
    SEV_DECL IpEndPoint(const struct sockaddr* sock_addr, uint32_t size);
    SEV_DECL IpEndPoint(const IpEndPoint& other);
    SEV_DECL ~IpEndPoint();

public:
    SEV_DECL AddressFamily getFamily() const;

    SEV_DECL bool isIpv4() const
    {
        return (getFamily() == AddressFamily::Ipv4);
    }

    SEV_DECL bool isIpv6() const
    {
        return (getFamily() == AddressFamily::Ipv6);
    }

    SEV_DECL bool isUnspec() const
    {
        return (getFamily() == AddressFamily::Unspec);
    }

    SEV_DECL std::string getAddress() const;
    SEV_DECL void setAddress(const std::string& address);

    SEV_DECL uint16_t getPort() const;
    SEV_DECL void setPort(uint16_t port);

    SEV_DECL std::string toString() const;

    SEV_DECL void clear();

public:
    SEV_DECL struct sockaddr* getTable()
    {
        return reinterpret_cast<struct sockaddr*>(&mSockAddr);
    }

    SEV_DECL const struct sockaddr* getTable() const
    {
        return reinterpret_cast<const struct sockaddr*>(&mSockAddr);
    }

    SEV_DECL uint32_t getTableSize() const;

    SEV_DECL IpEndPoint& operator=(const IpEndPoint& other);

public:
    SEV_DECL static std::list<IpEndPoint> resolveName(
        const std::string& node, uint16_t port,
        const AddressFamily& family, const Socket::Type& type,
        uint32_t flags = 0);

    SEV_DECL static std::list<IpEndPoint> resolveService(
        const std::string& node, const std::string& service,
        const AddressFamily& family, const Socket::Type& type,
        uint32_t flags = 0);

private:
    union SockAddr
    {
        struct sockaddr_in v4;
        struct sockaddr_in6 v6;
        struct sockaddr_storage any;

    } mSockAddr;
};

SEV_NS_END

#endif // SUBEVENT_SOCKET_HPP
