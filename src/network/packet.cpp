// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <arpa/inet.h>
#include <string>
#include "network/packet.h"

Packet::Packet() :
read_pos(0),
is_valid(true)
{}

void Packet::Append(const void* in_data, std::size_t size_in_bytes)
{
    if (in_data && (size_in_bytes > 0))
    {
        std::size_t start = data.size();
        data.resize(start + size_in_bytes);
        std::memcpy(&data[start], in_data, size_in_bytes);
    }
}

void Packet::Clear()
{
    data.clear();
    read_pos = 0;
    is_valid = true;
}

const void* Packet::GetData() const
{
    return !data.empty() ? &data[0] : NULL;
}

void Packet::IgnoreBytes(uint32_t length) {
    read_pos += length;
}

std::size_t Packet::GetDataSize() const
{
    return data.size();
}

bool Packet::EndOfPacket() const
{
    return read_pos >= data.size();
}

Packet::operator BoolType() const
{
    return is_valid ? &Packet::CheckSize : NULL;
}

Packet& Packet::operator >>(bool& out_data)
{
    uint8_t value;
    if (*this >> value)
        out_data = (value != 0);

    return *this;
}

Packet& Packet::operator >>(int8_t& out_data)
{
    if (CheckSize(sizeof(out_data)))
    {
        out_data = *reinterpret_cast<const int8_t*>(&data[read_pos]);
        read_pos += sizeof(out_data);
    }

    return *this;
}

Packet& Packet::operator >>(uint8_t& out_data)
{
    if (CheckSize(sizeof(out_data)))
    {
        out_data = *reinterpret_cast<const uint8_t*>(&data[read_pos]);
        read_pos += sizeof(out_data);
    }

    return *this;
}

Packet& Packet::operator >>(int16_t& out_data)
{
    if (CheckSize(sizeof(out_data)))
    {
        out_data = ntohs(*reinterpret_cast<const int16_t*>(&data[read_pos]));
        read_pos += sizeof(out_data);
    }

    return *this;
}

Packet& Packet::operator >>(uint16_t& out_data)
{
    if (CheckSize(sizeof(out_data)))
    {
        out_data = ntohs(*reinterpret_cast<const uint16_t*>(&data[read_pos]));
        read_pos += sizeof(out_data);
    }

    return *this;
}

Packet& Packet::operator >>(int32_t& out_data)
{
    if (CheckSize(sizeof(out_data)))
    {
        out_data = ntohl(*reinterpret_cast<const int32_t*>(&data[read_pos]));
        read_pos += sizeof(out_data);
    }

    return *this;
}

Packet& Packet::operator >>(uint32_t& out_data)
{
    if (CheckSize(sizeof(out_data)))
    {
        out_data = ntohl(*reinterpret_cast<const uint32_t*>(&data[read_pos]));
        read_pos += sizeof(out_data);
    }

    return *this;
}

Packet& Packet::operator >>(float& out_data)
{
    if (CheckSize(sizeof(out_data)))
    {
        out_data = *reinterpret_cast<const float*>(&data[read_pos]);
        read_pos += sizeof(out_data);
    }

    return *this;
}

Packet& Packet::operator >>(double& out_data)
{
    if (CheckSize(sizeof(out_data)))
    {
        out_data = *reinterpret_cast<const double*>(&data[read_pos]);
        read_pos += sizeof(out_data);
    }

    return *this;
}

Packet& Packet::operator >>(char* out_data)
{
    // First extract string length
    uint32_t length = 0;
    *this >> length;

    if ((length > 0) && CheckSize(length))
    {
        // Then extract characters
        std::memcpy(out_data, &data[read_pos], length);
        out_data[length] = '\0';

        // Update reading position
        read_pos += length;
    }

    return *this;
}

Packet& Packet::operator >>(std::string& out_data)
{
    // First extract string length
    uint32_t length = 0;
    *this >> length;

    out_data.clear();
    if ((length > 0) && CheckSize(length))
    {
        // Then extract characters
        out_data.assign(&data[read_pos], length);

        // Update reading position
        read_pos += length;
    }

    return *this;
}

Packet& Packet::operator <<(bool in_data)
{
    *this << static_cast<uint8_t>(in_data);
    return *this;
}


Packet& Packet::operator <<(int8_t in_data)
{
    Append(&in_data, sizeof(in_data));
    return *this;
}

Packet& Packet::operator <<(uint8_t in_data)
{
    Append(&in_data, sizeof(in_data));
    return *this;
}

Packet& Packet::operator <<(int16_t in_data)
{
    int16_t to_write = htons(in_data);
    Append(&to_write, sizeof(to_write));
    return *this;
}


Packet& Packet::operator <<(uint16_t in_data)
{
    uint16_t to_write = htons(in_data);
    Append(&to_write, sizeof(to_write));
    return *this;
}


Packet& Packet::operator <<(int32_t in_data)
{
    int32_t to_write = htonl(in_data);
    Append(&to_write, sizeof(to_write));
    return *this;
}


Packet& Packet::operator <<(uint32_t in_data)
{
    uint32_t to_write = htonl(in_data);
    Append(&to_write, sizeof(to_write));
    return *this;
}

Packet& Packet::operator <<(float in_data)
{
    Append(&in_data, sizeof(in_data));
    return *this;
}

Packet& Packet::operator <<(double in_data)
{
    Append(&in_data, sizeof(in_data));
    return *this;
}

Packet& Packet::operator <<(const char* in_data)
{
    // First insert string length
    uint32_t length = std::strlen(in_data);
    *this << length;

    // Then insert characters
    Append(in_data, length*sizeof(char));

    return *this;
}

Packet& Packet::operator <<(const std::string& in_data)
{
    // First insert string length
    uint32_t length = static_cast<uint32_t>(in_data.size());
    *this << length;

    // Then insert characters
    if (length > 0)
        Append(in_data.c_str(), length*sizeof(std::string::value_type));

    return *this;
}


bool Packet::CheckSize(std::size_t size)
{
    is_valid = is_valid && (read_pos + size <= data.size());
    return is_valid;
}
