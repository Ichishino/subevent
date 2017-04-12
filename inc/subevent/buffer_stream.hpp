#ifndef SUBEVENT_BUFFER_STREAM_HPP
#define SUBEVENT_BUFFER_STREAM_HPP

#include <vector>
#include <string>
#include <cstring>

#include <subevent/std.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// IBufferStream
//----------------------------------------------------------------------------//

class IBufferStream
{
public:
    SEV_DECL IBufferStream(const std::vector<char>& buff)
        : mBuff(buff)
    {
        mCur = 0;
    }

    SEV_DECL virtual ~IBufferStream()
    {
    }

public:
    SEV_DECL const char* getPtr() const
    {
        return &mBuff[mCur];
    }

    SEV_DECL const std::vector<char>& getBuffer() const
    {
        return mBuff;
    }

    SEV_DECL size_t getSize() const
    {
        return mBuff.size();
    }

    SEV_DECL bool isEnd() const
    {
        return (mCur >= getSize());
    }

    SEV_DECL size_t getReadableSize() const
    {
        return (getSize() - mCur);
    }

    SEV_DECL size_t getCur() const
    {
        return mCur;
    }

    SEV_DECL void setCur(size_t cur)
    {
        mCur = cur;
    }

    SEV_DECL void seekCur(int32_t num)
    {
        mCur += num;
    }

public:
    SEV_DECL bool readBytes(void* buff, size_t size)
    {
        if (getReadableSize() < size)
        {
            return false;
        }

        memcpy(buff, getPtr(), size);
        seekCur(static_cast<int32_t>(size));

        return true;
    }

    template <typename BuffType>
    SEV_DECL bool readBytes(
        BuffType& buff, const void* delim, size_t delimSize)
    {
        size_t readableSize = getReadableSize();

        for (size_t index = 0; index < readableSize; ++index)
        {
            const char* ptr = &getPtr()[index];

            if (memcmp(ptr, delim, delimSize) != 0)
            {
                continue;
            }

            size_t size = ptr - getPtr();

            if (size > 0)
            {
                buff.resize(size);

                memcpy(&buff[0], getPtr(), buff.size());
                seekCur(static_cast<int32_t>(buff.size()));
            }

            return true;
        }

        return false;
    }

public:
    SEV_DECL IBufferStream& operator>>(bool& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL IBufferStream& operator>>(int8_t& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL IBufferStream& operator>>(int16_t& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL IBufferStream& operator>>(int32_t& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL IBufferStream& operator>>(int64_t& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL IBufferStream& operator>>(uint8_t& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL IBufferStream& operator>>(uint16_t& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL IBufferStream& operator>>(uint32_t& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL IBufferStream& operator>>(uint64_t& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL IBufferStream& operator>>(double& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL IBufferStream& operator>>(float& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL IBufferStream& operator>>(std::string& data)
    {
        if (readBytes(data, "\0", 1))
        {
            seekCur(+1);
        }

        return *this;
    }

private:
    const std::vector<char>& mBuff;
    size_t mCur;
};

//----------------------------------------------------------------------------//
// OBufferStream
//----------------------------------------------------------------------------//

class OBufferStream
{
public:
    SEV_DECL OBufferStream(std::vector<char>& buff)
        : mBuff(buff)
    {
        mCur = 0;
    }

    SEV_DECL virtual ~OBufferStream()
    {
    }

public:
    SEV_DECL char* getPtr()
    {
        return &mBuff[mCur];
    }

    SEV_DECL const std::vector<char>& getBuffer() const
    {
        return mBuff;
    }

    SEV_DECL size_t getSize() const
    {
        return mBuff.size();
    }

    SEV_DECL bool isEnd() const
    {
        return (mCur >= getSize());
    }

    SEV_DECL size_t getWritableSize() const
    {
        return (getSize() - mCur);
    }

    SEV_DECL size_t getCur() const
    {
        return mCur;
    }

    SEV_DECL void setCur(size_t cur)
    {
        mCur = cur;
    }

    SEV_DECL void seekCur(int32_t num)
    {
        mCur += num;
    }

public:
    SEV_DECL void writeBytes(const void* data, size_t size)
    {
        if (getWritableSize() < size)
        {
            size_t shortage = size - getWritableSize();
            mBuff.resize(getSize() + shortage);
        }

        memcpy(getPtr(), data, size);
        seekCur(static_cast<int32_t>(size));
    }

public:
    SEV_DECL OBufferStream& operator<<(bool data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL OBufferStream& operator<<(int8_t data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL OBufferStream& operator<<(int16_t data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL OBufferStream& operator<<(int32_t data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL OBufferStream& operator<<(int64_t data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL OBufferStream& operator<<(uint8_t data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL OBufferStream& operator<<(uint16_t data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL OBufferStream& operator<<(uint32_t data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL OBufferStream& operator<<(uint64_t data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL OBufferStream& operator<<(double data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL OBufferStream& operator<<(float data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL OBufferStream& operator<<(const std::string& data)
    {
        writeBytes(data.c_str(),
            (data.size() + 1) * sizeof(std::string::value_type));
        return *this;
    }

    SEV_DECL OBufferStream& operator<<(const char* data)
    {
        writeBytes(data, (strlen(data) + 1) * sizeof(char));
        return *this;
    }

private:
    std::vector<char>& mBuff;
    size_t mCur;
};

//----------------------------------------------------------------------------//
// IStringStream
//----------------------------------------------------------------------------//

class IStringStream : public IBufferStream
{
public:
    using IBufferStream::IBufferStream;

public:
    SEV_DECL bool readString(
        std::string& str, const char* delim)
    {
        return readBytes(str, delim, strlen(delim));
    }

    SEV_DECL std::string readString(size_t size = SIZE_MAX)
    {
        std::string str;

        size_t readableSize = getReadableSize();

        if (size > readableSize)
        {
            size = readableSize;
        }

        if (size > 0)
        {
            str.resize(size);
            readBytes(&str[0], size);
        }

        return str;
    }
};

//----------------------------------------------------------------------------//
// OStringStream
//----------------------------------------------------------------------------//

class OStringStream : public OBufferStream
{
public:
    using OBufferStream::OBufferStream;

public:
    SEV_DECL void writeString(const std::string& str)
    {
        size_t charSize =
            sizeof(std::string::value_type);

        writeBytes(
            str.c_str(), ((str.size() + 1) * charSize));

        seekCur(-static_cast<int32_t>(charSize));
    }

public:
    SEV_DECL OStringStream& operator<<(bool data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL OStringStream& operator<<(int8_t data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL OStringStream& operator<<(int16_t data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL OStringStream& operator<<(int32_t data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL OStringStream& operator<<(int64_t data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL OStringStream& operator<<(uint8_t data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL OStringStream& operator<<(uint16_t data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL OStringStream& operator<<(uint32_t data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL OStringStream& operator<<(uint64_t data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL OStringStream& operator<<(double data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL OStringStream& operator<<(float data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL OStringStream& operator<<(const std::string& data)
    {
        writeString(data);
        return *this;
    }

    SEV_DECL OStringStream& operator<<(const char* data)
    {
        writeString(data);
        return *this;
    }
};

SEV_NS_END

#endif // SUBEVENT_BUFFER_STREAM_HPP
