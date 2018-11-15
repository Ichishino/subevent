#ifndef SUBEVENT_WS_INL
#define SUBEVENT_WS_INL

#include <string>
#include <cstring>

#include <subevent/ws.hpp>
#include <subevent/net_byte_io.hpp>
#include <subevent/network.hpp>

SEV_NS_BEGIN

//---------------------------------------------------------------------------//
// WsFrame
//---------------------------------------------------------------------------//

WsFrame::WsFrame(bool mask)
    : WsFrame(OpCode::Binary, mask)
{
}

WsFrame::WsFrame(uint8_t opCode, bool mask)
{
    clear();

    mOpCode = opCode;
    mMask = mask;

    if (mask)
    {
        generateMaskingKey();
    }
}

WsFrame::WsFrame(const WsFrame& other)
{
    operator=(other);
}

WsFrame::WsFrame(WsFrame&& other)
{
    operator=(std::move(other));
}

WsFrame::~WsFrame()
{
}

WsFrame& WsFrame::operator=(const WsFrame& other)
{
    mFin = other.mFin;
    mOpCode = other.mOpCode;
    mMask = other.mMask;
    mPayloadLength = other.mPayloadLength;
    mMaskingKey = other.mMaskingKey;
    mPayload = other.mPayload;

    return *this;
}

WsFrame& WsFrame::operator=(WsFrame&& other)
{
    mFin = std::move(other.mFin);
    mOpCode = std::move(other.mOpCode);
    mMask = std::move(other.mMask);
    mPayloadLength = std::move(other.mPayloadLength);
    mMaskingKey = std::move(other.mMaskingKey);
    mPayload = std::move(other.mPayload);

    other.clear();

    return *this;
}

void WsFrame::clear()
{
    mFin = true;
    mOpCode = OpCode::Binary;
    mMask = false;
    mMaskingKey.fill(0);
    mPayloadLength = 0;
    mPayload.clear();
}

void WsFrame::serializeHeader(ByteWriter& writer) const
{
    unsigned char b = 0x00;

    if (mFin)
    {
        b |= 0x80;
    }

    b |= (mOpCode & 0x0F);

    writer << b;
    b = 0x00;

    if (mMask)
    {
        b |= 0x80;
    }

    if (mPayloadLength > UINT16_MAX)
    {
        b |= 127;
        writer << b << (uint64_t)mPayloadLength;
    }
    else if (mPayloadLength > 125)
    {
        b |= 126;
        writer << b << (uint16_t)mPayloadLength;
    }
    else
    {
        b |= (uint8_t)mPayloadLength;
        writer << b;
    }

    if (mMask)
    {
        writer.writeBytes(&mMaskingKey[0], mMaskingKey.size());
    }
}

void WsFrame::serializePayload(ByteWriter& writer) const
{
    const auto payloadLength = getPayloadLength();

    if (payloadLength == 0)
    {
        return;
    }

    if (mMask)
    {
        const auto& maskingKey = getMaskingKey();

        for (size_t index = 0; index < payloadLength; ++index)
        {
            unsigned char b =
                mPayload[index] ^ maskingKey[index % 4];

            writer.writeBytes(&b, sizeof(b));
        }
    }
    else
    {
        writer.writeBytes(&mPayload[0], mPayload.size());
    }
}

bool WsFrame::deserializeHeader(ByteReader& reader)
{
    unsigned char bytes[getMinLength()];

    if (!reader.readBytes(bytes, getMinLength()))
    {
        return false;
    }

    mFin = bytes[0] & 0x80;
    mOpCode = bytes[0] & 0x0F;
    mMask = bytes[1] & 0x80;

    uint8_t len8 = bytes[1] & 0x7F;

    if (len8 == 127)
    {
        if (reader.getReadableSize() < sizeof(uint64_t))
        {
            return false;
        }

        reader >> mPayloadLength;
    }
    else if (len8 == 126)
    {
        if (reader.getReadableSize() < sizeof(uint16_t))
        {
            return false;
        }

        uint16_t len16;
        reader >> len16;

        mPayloadLength = len16;
    }
    else
    {
        mPayloadLength = len8;
    }

    if (isMask())
    {
        if (!reader.readBytes(&mMaskingKey[0], mMaskingKey.size()))
        {
            return false;
        }
    }

    return true;
}

bool WsFrame::deserializePayload(ByteReader& reader)
{
    if (reader.getReadableSize() < getPayloadLength())
    {
        return false;
    }

    const auto payloadLength = getPayloadLength();
    mPayload.resize(static_cast<size_t>(payloadLength));

    if (payloadLength > 0)
    {
        if (mMask)
        {
            const auto& maskingKey = getMaskingKey();

            for (size_t index = 0; index < payloadLength; ++index)
            {
                unsigned char b;
                reader.readBytes(&b, sizeof(b));

                mPayload[index] = b ^ maskingKey[index % 4];
            }
        }
        else
        {
            reader.readBytes(
                &mPayload[0], static_cast<size_t>(payloadLength));
        }
    }
    
    return true;
}

//----------------------------------------------------------------------------//
// WsChannel
//----------------------------------------------------------------------------//

WsChannel::WsChannel(const TcpChannelPtr& channel, bool isClient)
    : mIsClient(isClient), mChannel(channel)
{
    mOldReceiveHandler = channel->getReceiveHandler();

    channel->setReceiveHandler(
        SEV_BIND_1(this, WsChannel::onTcpReceive));
    channel->setCloseHandler(
        SEV_BIND_1(this, WsChannel::onTcpClose));
}

WsChannel::~WsChannel()
{
}

void WsChannel::close(uint16_t statusCode)
{
    if (isClosed () || isSentCloseFrame())
    {
        return;
    }

    send(WsCloseFrame(statusCode, isClientChannel()));

    mCloseState.mSent = true;

    if (mCloseState.mFrame != nullptr)
    {
        mCloseState.mClosed = true;
        mCloseState.mFrame.reset();
    }
}

int32_t WsChannel::send(
    const WsFrame& frame,
    const void* payload,
    const TcpSendHandler& handler)
{
    if (isClosed() || isSentCloseFrame())
    {
        return -1;
    }

    TcpChannelPtr channel = mChannel.lock();

    if (channel == nullptr)
    {
        return  -2;
    }

    std::vector<char> sendData;

    NetByteWriter writer(sendData);

    frame.serializeHeader(writer);

    if (payload != nullptr)
    {
        writer.writeBytes(
            payload, static_cast<size_t>(frame.getPayloadLength()));
    }
    else
    {
        frame.serializePayload(writer);
    }
    
    return channel->send(std::move(sendData), handler);
}

int32_t WsChannel::send(
    const void* data, size_t size,
    const TcpSendHandler& handler)
{
    WsFrame frame(WsFrame::OpCode::Binary, isClientChannel());
    frame.setPayloadLength(size);
    
    return send(frame, data, handler);
}

int32_t WsChannel::send(
    const std::vector<char>& payload,
    const TcpSendHandler& handler)
{
    return send(&payload[0], payload.size(), handler);
}

int32_t WsChannel::send(
    const std::string& payload,
    const TcpSendHandler& handler)
{
    WsFrame frame(WsFrame::OpCode::Text, isClientChannel());
    frame.setPayloadLength(payload.size());

    return send(frame, payload.c_str(), handler);
}

int32_t WsChannel::sendPing(
    const void* payload, size_t size,
    const TcpSendHandler& handler)
{
    WsFrame frame(WsFrame::OpCode::Ping, isClientChannel());
    frame.setPayloadLength(size);

    return send(frame, payload, handler);
}

void WsChannel::onTcpReceive(const TcpChannelPtr& channel)
{
    auto message = channel->receiveAll();

    if (mReceiveCache.mPosition > 0)
    {
        mReceiveCache.mBuffer.insert(
            mReceiveCache.mBuffer.end(),
            message.begin(), message.end());
    }
    else
    {
        mReceiveCache.mBuffer = std::move(message);
    }

    NetByteReader reader(mReceiveCache.mBuffer);
    reader.setCur(mReceiveCache.mPosition);

    uint32_t headPosition =
        (uint32_t)mReceiveCache.mPosition;

    std::list<WsFramePtr> receiveFrames;

    for (;;)
    {
        uint32_t totalLength =
            (uint32_t)(mReceiveCache.mBuffer.size() - headPosition);

        if (totalLength < WsFrame::getMinLength())
        {
            break;
        }

        size_t size = reader.getReadableSize();

        WsFramePtr frame = std::make_shared<WsFrame>();

        try
        {
            if (!frame->deserializeHeader(reader))
            {
                break;
            }

            if (!frame->deserializePayload(reader))
            {
                break;
            }

            if (frame->isControlFrame())
            {
                if (frame->getOpCode() == WsFrame::OpCode::ConnectionClose)
                { 
                    mCloseState.mFrame = frame;
                }
                else
                {
                    receiveFrames.push_back(frame);
                }
            }
            else if (mContinuationFrame != nullptr)
            {
                mContinuationFrame->addPayload(frame->getPayload());

                if (frame->isFin())
                {
                    mContinuationFrame->setFin(true);
                    receiveFrames.push_back(mContinuationFrame);
                    mContinuationFrame.reset();
                }
            }
            else
            {
                if (frame->isFin())
                {
                    receiveFrames.push_back(frame);
                }
                else
                {
                    mContinuationFrame = frame;
                }
            }
        }
        catch (...)
        {
            // error
            return;
        }

        headPosition +=
            (uint32_t)(size - reader.getReadableSize());
    }

    if (reader.getReadableSize() == 0)
    {
        mReceiveCache.clear();
    }
    else
    {
        mReceiveCache.mPosition = headPosition;
    }

    if (mCloseState.mFrame != nullptr)
    {
        if (mContinuationFrame == nullptr)
        {
            receiveFrames.push_back(mCloseState.mFrame);
        }
    }

    for (const auto& frame : receiveFrames)
    {
        onReceiveFrame(frame);
    }
}

void WsChannel::onTcpClose(const TcpChannelPtr& channel)
{
    if (mCloseHandler != nullptr)
    {
        mCloseHandler(channel);
    }
}

void WsChannel::onReceiveFrame(const WsFramePtr& frame)
{
    WsChannelPtr self =
        std::dynamic_pointer_cast<WsChannel>(shared_from_this());

    if (frame->isControlFrame())
    {        
        if (mControlFrameHandler != nullptr)
        {
            mControlFrameHandler(self, frame);
        }
        else
        {
            onControlFrame(frame);
        }
    }
    else
    {
        if (mDataFrameHandler != nullptr)
        {
            mDataFrameHandler(self, frame);
        }
    }
}

void WsChannel::onControlFrame(const WsFramePtr& frame)
{
    // default handler

    switch (frame->getOpCode())
    {
    case WsFrame::OpCode::Ping:
    {
        if (isClosed() || isSentCloseFrame())
        {
            return;
        }

        WsFrame pongFrame(WsFrame::OpCode::Pong, isClientChannel());
        pongFrame.setPayload(frame->getPayload());

        send(pongFrame);

        break;
    }
    case WsFrame::OpCode::Pong:
    {
        break;
    }
    case WsFrame::OpCode::ConnectionClose:
    {
        if (isClosed())
        {
            return;
        }

        if (!isSentCloseFrame())
        {
            uint16_t statusCode =
                WsCloseFrame::StatusCode::NoStatusReceived;

            if (frame->getPayloadLength() >= sizeof(uint16_t))
            {
                NetByteReader reader(frame->getPayload());
                reader >> statusCode;
            }

            send(WsCloseFrame(statusCode, isClientChannel()));

            mCloseState.mSent = true;
            mCloseState.mFrame.reset();
        }

        mCloseState.mClosed = true;

        TcpChannelPtr channel = mChannel.lock();

        if (channel != nullptr)
        {
            channel->setReceiveHandler(mOldReceiveHandler);
        }

        break;
    }
    default:
        break;
    }
}

SEV_NS_END

#endif // SUBEVENT_WS_INL
