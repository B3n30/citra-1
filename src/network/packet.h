// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <vector>

class Packet {
    // A bool-like type that cannot be converted to integer or pointer types
    typedef bool (Packet::*BoolType)(std::size_t);

public:
    Packet();
    ~Packet() = default;

    /**
     *  Append data to the end of the packet
     * @param data        Pointer to the sequence of bytes to append
     * @param sizeInBytes Number of bytes to append
     */
    void Append(const void* in_data, std::size_t sizeInBytes);

    /**
     * Clear the packet
     * After calling Clear, the packet is empty.
     */
    void Clear();

    /**
     * Skip bytes to read
     * @param length  Number of bytes to skip
     */
    void IgnoreBytes(uint32_t length);

    /**
     * Get a pointer to the data contained in the packet
     *
     * @return Pointer to the data
     */
    const void* GetData() const;

    /**
     * Get the size of the data contained in the packet
     *
     * This function returns the number of bytes pointed to by what getData returns.
     * @return Data size, in bytes
     */
    std::size_t GetDataSize() const;

    /**
     * Tell if the reading position has reached the end of the packet
     *
     * This function is useful to know if there is some data
     * left to be read, without actually reading it.
     * @ return True if all data was read, false otherwise
     */
    bool EndOfPacket() const;

    /**
     * Test the validity of the packet, for reading
     *
     * This operator allows to test the packet as a boolean variable, to check if a reading
     * operation was successful.
     * A packet will be in an invalid state if it has no more data to read.
     *  This behaviour is the same as standard C++ streams.
     * Usage example:
     * \code
     * float x;
     * packet >> x;
     * if (packet) {
     *     // ok, x was extracted successfully
     * }
     *
     *  // -- or --
     *
     * float x;
     * if (packet >> x) {
     *     // ok, x was extracted successfully
     * }
     * \endcode
     *
     * Don't focus on the return type, it's equivalent to bool but
     * it disallows unwanted implicit conversions to integer or
     * pointer types.
     *
     * @return True if last data extraction from packet was successful
     */
    operator BoolType() const;

    /**
     * Overloads of operator >> to read data from the packet
     */
    Packet& operator>>(bool& out_data);
    Packet& operator>>(int8_t& out_data);
    Packet& operator>>(uint8_t& out_data);
    Packet& operator>>(int16_t& out_data);
    Packet& operator>>(uint16_t& out_data);
    Packet& operator>>(int32_t& out_data);
    Packet& operator>>(uint32_t& out_data);
    Packet& operator>>(float& out_data);
    Packet& operator>>(double& out_data);
    Packet& operator>>(char* out_data);
    Packet& operator>>(std::string& out_data);
    template <typename T>
    Packet& operator>>(std::vector<T>& out_data);
    template <typename T, std::size_t S>
    Packet& operator>>(std::array<T, S>& out_data);

    /**
     * Overloads of operator << to write data into the packet
     */
    Packet& operator<<(bool in_data);
    Packet& operator<<(int8_t in_data);
    Packet& operator<<(uint8_t in_data);
    Packet& operator<<(int16_t in_data);
    Packet& operator<<(uint16_t in_data);
    Packet& operator<<(int32_t in_data);
    Packet& operator<<(uint32_t in_data);
    Packet& operator<<(float in_data);
    Packet& operator<<(double in_data);
    Packet& operator<<(const char* in_data);
    Packet& operator<<(const std::string& in_data);
    template <typename T>
    Packet& operator<<(const std::vector<T>& in_data);
    template <typename T, std::size_t S>
    Packet& operator<<(const std::array<T, S>& in_data);

private:
    /**
     * Disallow comparisons between packets
     */
    bool operator==(const Packet& right) const;
    bool operator!=(const Packet& right) const;

    /** Check if the packet can extract a given number of bytes
     *
     * This function updates accordingly the state of the packet.
     * @param size Size to check
     * @return True if size bytes can be read from the packet
     */
    bool CheckSize(std::size_t size);

    std::vector<char> data; ///< Data stored in the packet
    std::size_t read_pos;   ///< Current reading position in the packet
    bool is_valid;          ///< Reading state of the packet
};

template <typename T>
Packet& Packet::operator>>(std::vector<T>& out_data) {
    for (uint32_t i = 0; i < out_data.size(); ++i) {
        *this >> out_data[i];
    }
    return *this;
}

template <typename T, std::size_t S>
Packet& Packet::operator>>(std::array<T, S>& out_data) {
    for (uint32_t i = 0; i < out_data.size(); ++i) {
        *this >> out_data[i];
    }
    return *this;
}

template <typename T>
Packet& Packet::operator<<(const std::vector<T>& in_data) {
    for (uint32_t i = 0; i < in_data.size(); ++i) {
        *this << in_data[i];
    }
    return *this;
}

template <typename T, std::size_t S>
Packet& Packet::operator<<(const std::array<T, S>& in_data) {
    for (uint32_t i = 0; i < in_data.size(); ++i) {
        *this << in_data[i];
    }
    return *this;
}
