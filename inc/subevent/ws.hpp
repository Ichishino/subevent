#ifndef SUBEVENT_WS_HPP
#define SUBEVENT_WS_HPP

#include <array>
#include <vector>
#include <string>
#include <memory>
#include <functional>

#include <subevent/std.hpp>
#include <subevent/byte_io.hpp>
#include <subevent/utility.hpp>
#include <subevent/tcp.hpp>
#include <subevent/http_server.hpp>

SEV_NS_BEGIN

typedef std::function<
    void(const WsChannelPtr&, const WsFramePtr&)> WsReceiveHandler;

#define SEV_WS_KEY_SUFFIX   "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

//---------------------------------------------------------------------------//
// WsFrame
//---------------------------------------------------------------------------//

class WsFrame
{
public:
    SEV_DECL explicit WsFrame(bool mask = false);
    SEV_DECL explicit WsFrame(uint8_t opCode, bool mask = false);
    SEV_DECL WsFrame(const WsFrame& other);
    SEV_DECL WsFrame(WsFrame&& other);

    SEV_DECL virtual ~WsFrame();

    struct OpCode
    {
        static const uint8_t Continuation = 0x00;
        static const uint8_t Text = 0x01;
        static const uint8_t Binary = 0x02;

        static const uint8_t ConnectionClose = 0x08;
        static const uint8_t Ping = 0x09;
        static const uint8_t Pong = 0x0A;
    };

public:
    SEV_DECL void setFin(bool fin)
    {
        mFin = fin;
    }

    SEV_DECL bool isFin() const
    {
        return mFin;
    }

    SEV_DECL void setOpCode(uint8_t opCode)
    {
        mOpCode = opCode;
    }

    SEV_DECL uint8_t getOpCode() const
    {
        return mOpCode;
    }

    SEV_DECL bool isControlFrame() const
    {
        return ((getOpCode() & 0x08) > 0);
    }

    SEV_DECL void setMask(bool mask)
    {
        mMask = mask;
    }

    SEV_DECL bool isMask() const
    {
        return mMask;
    }

    SEV_DECL void setPayloadLength(uint64_t payloadLength)
    {
        mPayloadLength = payloadLength;
    }

    SEV_DECL uint64_t getPayloadLength() const
    {
        return mPayloadLength;
    }

    SEV_DECL void setMaskingKey(unsigned const char* maskingKey)
    {
        mMaskingKey[0] = maskingKey[0];
        mMaskingKey[1] = maskingKey[1];
        mMaskingKey[2] = maskingKey[2];
        mMaskingKey[3] = maskingKey[3];
    }

    SEV_DECL const std::array<unsigned char, 4>& getMaskingKey() const
    {
        return mMaskingKey;
    }

    SEV_DECL void generateMaskingKey()
    {
        uint32_t random32 = Random::generate32();

        mMaskingKey[0] = (unsigned char)(random32 & 0x000000FF);
        mMaskingKey[1] = (unsigned char)((random32 & 0x0000FF00) >> 8);
        mMaskingKey[2] = (unsigned char)((random32 & 0x00FF0000) >> 16);
        mMaskingKey[3] = (unsigned char)((random32 & 0xFF000000) >> 24);
    }

    SEV_DECL void setPayload(const std::vector<char>& payload)
    {
        mPayload = payload;
        mPayloadLength = mPayload.size();
    }

    SEV_DECL void setPayload(std::vector<char>&& payload)
    {
        mPayload = std::move(payload);
        mPayloadLength = mPayload.size();
    }

    SEV_DECL void setPayload(const void* data, size_t size)
    {
        mPayload.resize(size);
        memcpy(&mPayload[0], data, size);
        mPayloadLength = mPayload.size();
    }

    SEV_DECL void addPayload(const std::vector<char>& payload)
    {
        mPayload.insert(mPayload.end(), payload.begin(), payload.end());
        mPayloadLength += payload.size();
    }

    SEV_DECL const std::vector<char>& getPayload() const
    {
        return mPayload;
    }

    SEV_DECL void clear();

public:
    SEV_DECL virtual void serializeHeader(ByteWriter& writer) const;
    SEV_DECL virtual void serializePayload(ByteWriter& writer) const;
    SEV_DECL virtual bool deserializeHeader(ByteReader& reader);
    SEV_DECL virtual bool deserializePayload(ByteReader& reader);

    SEV_DECL static constexpr uint32_t getMinLength()
    {
        return 2;
    }

    SEV_DECL WsFrame& operator=(const WsFrame& other);
    SEV_DECL WsFrame& operator=(WsFrame&& other);

protected:
    bool mFin;
    uint8_t mOpCode;
    bool mMask;
    uint64_t mPayloadLength;
    std::array<unsigned char, 4> mMaskingKey;
    std::vector<char> mPayload;
};

//---------------------------------------------------------------------------//
// WsCloseFrame
//---------------------------------------------------------------------------//

class WsCloseFrame : public WsFrame
{
public:
    SEV_DECL WsCloseFrame(bool mask = false)
        : WsCloseFrame(StatusCode::NormalClosure, mask)
    {
    }

    SEV_DECL WsCloseFrame(uint16_t statusCode, bool mask = false)
        : WsFrame(OpCode::ConnectionClose, mask)
    {
        mPayload.resize(sizeof(Payload));
        mPayloadLength = mPayload.size();
        setStatusCode(statusCode);
    }
    
    SEV_DECL ~WsCloseFrame() override
    {
    }

    struct Payload
    {
        uint16_t mStatusCode = 0;
    };

    struct StatusCode
    {
        static const uint16_t NormalClosure = 1000;
        static const uint16_t GoingAway = 1001;
        static const uint16_t ProtocolError = 1002;
        static const uint16_t UnsupportedData = 1003;

        static const uint16_t NoStatusReceived = 1005;
        static const uint16_t AbnormalClosure = 1006;
        static const uint16_t InvalidFramePayloadData = 1007;
        static const uint16_t PolicyViolation = 1008;
        static const uint16_t MessageTooBig = 1009;
        static const uint16_t MissingExtension = 1010;
        static const uint16_t InternalError = 1011;

        static const uint16_t TLSHandshake = 1015;
    };

public:
    SEV_DECL void setStatusCode(uint16_t statusCode)
    {
        getPayload()->mStatusCode = statusCode;
    }

    SEV_DECL uint16_t getStatusCode() const
    {
        return getPayload()->mStatusCode;
    }

public:
    SEV_DECL void serializePayload(ByteWriter& writer) const override
    {
        writer << getStatusCode();
    }

private:
    SEV_DECL Payload* getPayload()
    {
        return reinterpret_cast<Payload*>(&mPayload[0]);
    }
    
    SEV_DECL const Payload* getPayload() const
    {
        return reinterpret_cast<const Payload*>(&mPayload[0]);
    }
};

//----------------------------------------------------------------------------//
// WsChannel
//----------------------------------------------------------------------------//

class WsChannel : public std::enable_shared_from_this<WsChannel>
{
public:
    SEV_DECL static WsChannelPtr
        newInstance(const TcpChannelPtr& channel, bool isClient)
    {
        return std::make_shared<WsChannel>(channel, isClient);
    }

    SEV_DECL virtual ~WsChannel();

public:

    // binary
    SEV_DECL int32_t send(
        const void* payload, size_t size,
        const TcpSendHandler& handler = nullptr);
    SEV_DECL int32_t send(
        const std::vector<char>& payload,
        const TcpSendHandler& handler = nullptr);

    // text
    SEV_DECL int32_t send(
        const std::string& payload,
        const TcpSendHandler& handler = nullptr);

    SEV_DECL int32_t send(
        const WsFrame& frame,
        const void* payload = nullptr,
        const TcpSendHandler& handler = nullptr);

    SEV_DECL int32_t sendPing(
        const void* payload = nullptr, size_t size = 0,
        const TcpSendHandler& handler = nullptr);

    SEV_DECL void close(
        uint16_t statusCode = WsCloseFrame::StatusCode::NormalClosure);

    SEV_DECL void setDataFrameHandler(
        const WsReceiveHandler& handler)
    {
        mDataFrameHandler = handler;
    }

    SEV_DECL void setControlFrameHandler(
        const WsReceiveHandler& handler)
    {
        mControlFrameHandler = handler;
    }

    SEV_DECL void setCloseHandler(
        const TcpCloseHandler& handler)
    {
        mCloseHandler = handler;
    }

public:
    SEV_DECL bool isClosed() const
    {
        return mCloseState.mClosed;
    }

    SEV_DECL bool isSentCloseFrame() const
    {
        return mCloseState.mSent;
    }

    SEV_DECL void onControlFrame(const WsFramePtr& frame);

public:
    SEV_DECL WsChannel(const TcpChannelPtr& channel, bool isClient);

    SEV_DECL TcpChannelPtr getTcpChannel() const
    {
        return mChannel.lock();
    }

protected:
    SEV_DECL void onTcpReceive(const TcpChannelPtr& channel);
    SEV_DECL void onTcpClose(const TcpChannelPtr& channel);

    SEV_DECL bool isClientChannel() const
    {
        return mIsClient;
    }

private:
    WsChannel() = delete;
    WsChannel(const WsChannel&) = delete;
    WsChannel& operator=(const WsChannel&) = delete;

    SEV_DECL void onReceiveFrame(const WsFramePtr& frame);

    bool mIsClient;
    std::weak_ptr<TcpChannel> mChannel;
    WsReceiveHandler mDataFrameHandler;
    WsReceiveHandler mControlFrameHandler;
    TcpCloseHandler mCloseHandler;
    TcpReceiveHandler mOldReceiveHandler;

    struct ReceiveCache
    {
        size_t mPosition = 0;
        std::vector<char> mBuffer;

        void clear()
        {
            mPosition = 0;
            mBuffer.clear();
        }

    } mReceiveCache;

    struct CloseState
    {
        bool mClosed = false;
        bool mSent = false;
        WsFramePtr mFrame;
    
    } mCloseState;

    WsFramePtr mContinuationFrame;

    friend class HttpChannel;

    friend bool operator==(
        const WsChannelPtr&, const TcpChannelPtr&);
    friend bool operator==(
        const WsChannelPtr&, const HttpChannelPtr&);
};

// operator for TcpChannelPtr
inline bool operator==(
    const WsChannelPtr& l, const TcpChannelPtr& r)
{
    return (l != nullptr) && (l->mChannel.lock() == r);
}

inline bool operator!=(
    const WsChannelPtr& l, const TcpChannelPtr& r)
{
    return !operator==(l, r);
}

inline bool operator==(
    const TcpChannelPtr& l, const WsChannelPtr& r)
{
    return operator==(r, l);
}

inline bool operator!=(
    const TcpChannelPtr& l, const WsChannelPtr& r)
{
    return !operator==(r, l);
}

// operator for HttpChannelPtr
inline bool operator==(
    const WsChannelPtr& l, const HttpChannelPtr& r)
{
    return (l != nullptr) && (l->mChannel.lock() == r);
}

inline bool operator!=(
    const WsChannelPtr& l, const HttpChannelPtr& r)
{
    return !operator==(l, r);
}

inline bool operator==(
    const HttpChannelPtr& l, const WsChannelPtr& r)
{
    return operator==(r, l);
}

inline bool operator!=(
    const HttpChannelPtr& l, const WsChannelPtr& r)
{
    return !operator==(r, l);
}

SEV_NS_END

#endif // SUBEVENT_WS_HPP
