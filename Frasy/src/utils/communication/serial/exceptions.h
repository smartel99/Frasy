/**
 * @file    exceptions.h
 * @author  Samuel Martel
 * @date    2022-12-12
 * @brief
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. If
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */

#ifndef FRASY_UTILS_COMM_SERIAL_EXCEPTIONS_H
#define FRASY_UTILS_COMM_SERIAL_EXCEPTIONS_H
#include "Brigerad/Core/Log.h"
#include "packet.h"

#include <exception>
#include <span>
#include <string_view>
#include <vector>
#include <format>

namespace Frasy::Serial {
class BasePacketException : public std::exception
{
public:
    explicit BasePacketException(const uint8_t* data, size_t len, std::string_view msg) noexcept
    : m_data(std::format("{}, Data: {}", msg, std::span {data, len}))
    {
    }

    [[nodiscard]] const char* what() const override { return m_data.c_str(); }

protected:
    std::string m_data;
};

class MissingDataException : public BasePacketException
{
public:
    MissingDataException(const uint8_t* data, size_t len, const char* from)
    : BasePacketException(data, len, std::format("Missing data for {}", from))
    {
    }
};

class BadDelimiterException : public BasePacketException
{
public:
    BadDelimiterException(const uint8_t* data, size_t len, const char* from, uint8_t expected)
    : BasePacketException(
        data, len, std::format("Bad delimiter for {}, expected '{:#02x}', got '{:#02x}'", from, expected, data[0]))
    {
    }
};

class BadPayloadException : public BasePacketException
{
public:
    BadPayloadException(const uint8_t* data, size_t len, size_t expectedIdx)
    : BasePacketException(data,
                          len,
                          std::format("Expected to find payload end at index {}, found '{:02X}' instead",
                                      expectedIdx,
                                      expectedIdx >= len ? '\xFF' : data[expectedIdx]))
    {
    }
};

class BadCrcException : public BasePacketException
{
public:
    BadCrcException(const uint8_t* data, size_t len, uint32_t expected, uint32_t computed)
    : BasePacketException(data, len, std::format("Bad CRC, expected {}, got {}", expected, computed))
    {
    }
};

}    // namespace Frasy::Communication

#endif    // FRASY_UTILS_COMM_SERIAL_EXCEPTIONS_H
