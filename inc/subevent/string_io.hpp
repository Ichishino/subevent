#ifndef SUBEVENT_STRING_IO_HPP
#define SUBEVENT_STRING_IO_HPP

#include <string>
#include <cstring>

#include <subevent/std.hpp>
#include <subevent/byte_io.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// StringReader
//----------------------------------------------------------------------------//

class StringReader : public ByteReader
{
public:
    SEV_DECL StringReader(const std::vector<char>& buff)
        : ByteReader(buff)
    {
    }

    SEV_DECL ~StringReader()
    {
    }

public:
    SEV_DECL bool readString(
        std::string& str, const char* delim,
        size_t maxSize = SIZE_MAX, bool* reachedMax = nullptr)
    {
        return readBytes(str, delim, strlen(delim), maxSize, reachedMax);
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
// StringWriter
//----------------------------------------------------------------------------//

class StringWriter : public ByteWriter
{
public:
    SEV_DECL StringWriter(std::vector<char>& buff)
        : ByteWriter(buff)
    {
    }

    SEV_DECL ~StringWriter()
    {
    }

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
    SEV_DECL StringWriter& operator<<(bool data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL StringWriter& operator<<(int8_t data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL StringWriter& operator<<(int16_t data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL StringWriter& operator<<(int32_t data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL StringWriter& operator<<(int64_t data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL StringWriter& operator<<(uint8_t data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL StringWriter& operator<<(uint16_t data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL StringWriter& operator<<(uint32_t data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL StringWriter& operator<<(uint64_t data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL StringWriter& operator<<(double data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL StringWriter& operator<<(float data)
    {
        writeString(std::to_string(data));
        return *this;
    }

    SEV_DECL StringWriter& operator<<(const std::string& data)
    {
        writeString(data);
        return *this;
    }

    SEV_DECL StringWriter& operator<<(const char* data)
    {
        writeString(data);
        return *this;
    }
};

SEV_NS_END

#endif // SUBEVENT_STRING_IO_HPP
