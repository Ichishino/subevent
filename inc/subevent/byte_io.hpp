#ifndef SUBEVENT_BYTE_IO_HPP
#define SUBEVENT_BYTE_IO_HPP

#include <vector>
#include <string>
#include <cstring>

#include <subevent/std.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// ByteReader
//----------------------------------------------------------------------------//

class ByteReader
{
public:
    SEV_DECL ByteReader(const std::vector<char>& buff)
        : mBuff(buff)
    {
        mCur = 0;
    }

    SEV_DECL virtual ~ByteReader()
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
        BuffType& buff, const void* delim, size_t delimSize,
        size_t maxSize = SIZE_MAX, bool* reachedMax = nullptr)
    {
        bool result = false;
        size_t lastIndex =
            (getReadableSize() < maxSize) ? getReadableSize() : maxSize;

        size_t index = 0;
        for (; index < lastIndex; ++index)
        {
            if (memcmp(&getPtr()[index], delim, delimSize) != 0)
            {
                continue;
            }

            buff.resize(index);

            if (index > 0)
            {
                memcpy(&buff[0], getPtr(), buff.size());
                seekCur(static_cast<int32_t>(buff.size()));
            }

            result = true;
            break;
        }

        if (reachedMax != nullptr)
        {
            *reachedMax = (index >= maxSize);
        }

        return result;
    }

public:
    SEV_DECL virtual ByteReader& operator>>(bool& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteReader& operator>>(int8_t& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteReader& operator>>(int16_t& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteReader& operator>>(int32_t& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteReader& operator>>(int64_t& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteReader& operator>>(uint8_t& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteReader& operator>>(uint16_t& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteReader& operator>>(uint32_t& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteReader& operator>>(uint64_t& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteReader& operator>>(double& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteReader& operator>>(float& data)
    {
        readBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteReader& operator>>(std::string& data)
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
// ByteWriter
//----------------------------------------------------------------------------//

class ByteWriter
{
public:
    SEV_DECL ByteWriter(std::vector<char>& buff)
        : mBuff(buff)
    {
        mCur = 0;
    }

    SEV_DECL virtual ~ByteWriter()
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
    SEV_DECL virtual ByteWriter& operator<<(bool data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteWriter& operator<<(int8_t data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteWriter& operator<<(int16_t data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteWriter& operator<<(int32_t data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteWriter& operator<<(int64_t data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteWriter& operator<<(uint8_t data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteWriter& operator<<(uint16_t data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteWriter& operator<<(uint32_t data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteWriter& operator<<(uint64_t data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteWriter& operator<<(double data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteWriter& operator<<(float data)
    {
        writeBytes(&data, sizeof(data));
        return *this;
    }

    SEV_DECL virtual ByteWriter& operator<<(const std::string& data)
    {
        writeBytes(data.c_str(),
            (data.size() + 1) * sizeof(std::string::value_type));
        return *this;
    }

    SEV_DECL virtual ByteWriter& operator<<(const char* data)
    {
        writeBytes(data, (strlen(data) + 1) * sizeof(char));
        return *this;
    }

private:
    std::vector<char>& mBuff;
    size_t mCur;
};

SEV_NS_END

#endif // SUBEVENT_BYTE_IO_HPP
