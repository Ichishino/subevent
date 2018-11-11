#ifndef SUBEVENT_NET_BYTE_IO_HPP
#define SUBEVENT_NET_BYTE_IO_HPP

#include <subevent/std.hpp>
#include <subevent/utility.hpp>
#include <subevent/byte_io.hpp>

SEV_NS_BEGIN

//----------------------------------------------------------------------------//
// NetByteReader
//----------------------------------------------------------------------------//

class NetByteReader : public ByteReader
{
public:
    SEV_DECL NetByteReader(const std::vector<char>& buff)
        : ByteReader(buff)
    {
    }

    SEV_DECL ~NetByteReader()
    {
    }

public:
    SEV_DECL NetByteReader & operator>>(int16_t& data)
    {
        ByteReader::operator>>(data);
        data = Endian::networkToHost(data);
        return *this;
    }

    SEV_DECL NetByteReader& operator>>(int32_t& data)
    {
        ByteReader::operator>>(data);
        data = Endian::networkToHost(data);
        return *this;
    }

    SEV_DECL NetByteReader& operator>>(int64_t& data)
    {
        ByteReader::operator>>(data);
        data = Endian::networkToHost(data);
        return *this;
    }

    SEV_DECL NetByteReader& operator>>(uint16_t& data)
    {
        ByteReader::operator>>(data);
        data = Endian::networkToHost(data);
        return *this;
    }

    SEV_DECL NetByteReader& operator>>(uint32_t& data)
    {
        ByteReader::operator>>(data);
        data = Endian::networkToHost(data);
        return *this;
    }

    SEV_DECL NetByteReader& operator>>(uint64_t& data)
    {
        ByteReader::operator>>(data);
        data = Endian::networkToHost(data);
        return *this;
    }

    SEV_DECL NetByteReader& operator>>(double& data)
    {
        ByteReader::operator>>(data);
        data = Endian::networkToHost(data);
        return *this;
    }

    SEV_DECL NetByteReader& operator>>(float& data)
    {
        ByteReader::operator>>(data);
        data = Endian::networkToHost(data);
        return *this;
    }
};

//----------------------------------------------------------------------------//
// NetByteWriter
//----------------------------------------------------------------------------//

class NetByteWriter : public ByteWriter
{
public:
    SEV_DECL NetByteWriter(std::vector<char>& buff)
        : ByteWriter(buff)
    {
    }

    SEV_DECL ~NetByteWriter()
    {
    }

public:
    SEV_DECL NetByteWriter& operator<<(int16_t data)
    {
        ByteWriter::operator<<(
            Endian::hostToNetwork(data));
        return *this;
    }

    SEV_DECL NetByteWriter& operator<<(int32_t data)
    {
        ByteWriter::operator<<(
            Endian::hostToNetwork(data));
        return *this;
    }

    SEV_DECL NetByteWriter& operator<<(int64_t data)
    {
        ByteWriter::operator<<(
            Endian::hostToNetwork(data));
        return *this;
    }

    SEV_DECL NetByteWriter& operator<<(uint16_t data)
    {
        ByteWriter::operator<<(
            Endian::hostToNetwork(data));
        return *this;
    }

    SEV_DECL NetByteWriter& operator<<(uint32_t data)
    {
        ByteWriter::operator<<(
            Endian::hostToNetwork(data));
        return *this;
    }

    SEV_DECL NetByteWriter& operator<<(uint64_t data)
    {
        ByteWriter::operator<<(
            Endian::hostToNetwork(data));
        return *this;
    }

    SEV_DECL NetByteWriter& operator<<(double data)
    {
        ByteWriter::operator<<(
            Endian::hostToNetwork(data));
        return *this;
    }

    SEV_DECL NetByteWriter& operator<<(float data)
    {
        ByteWriter::operator<<(
            Endian::hostToNetwork(data));
        return *this;
    }
};

SEV_NS_END

#endif // SUBEVENT_NET_BYTE_IO_HPP
