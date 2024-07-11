/**
 * @file    sdo_serializer.cpp
 * @author  Paul Thomas
 * @date    5/21/2024
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

#include "ode_serializer.h"
#include "../communication/can_open/types.h"

#include <utils/misc/serializer.h>

using DataType = Frasy::CanOpen::DataType;

static std::vector<uint8_t> serializeFloat(float value)
{
    auto                 size = sizeof(float);
    std::vector<uint8_t> result(size, 0);
    auto*                ptr = reinterpret_cast<uint8_t*>(&value);
    for (size_t i = 0; i < size; ++i) {
        // ReSharper disable once CppDFAUnreachableCode
        if constexpr (std::endian::native == std::endian::big) { result[size - i - 1] = ptr[i]; }
        else {
            result[i] = ptr[i];
        }
    }
    return result;
}

static std::vector<uint8_t> serializeDouble(double value)
{
    auto                 size = sizeof(double);
    std::vector<uint8_t> result(size, 0);
    auto*                ptr = reinterpret_cast<uint8_t*>(&value);
    for (size_t i = 0; i < size; ++i) {
        // ReSharper disable once CppDFAUnreachableCode
        if constexpr (std::endian::native == std::endian::big) { result[size - i - 1] = ptr[i]; }
        else {
            result[i] = ptr[i];
        }
    }
    return result;
}

static std::vector<uint8_t> serializeInteger(int64_t value, std::size_t size)
{
    std::vector<uint8_t> result(size, 0);
    auto*                ptr = reinterpret_cast<uint8_t*>(&value);
    for (size_t i = 0; i < size; ++i) {
        // ReSharper disable once CppDFAUnreachableCode
        if constexpr (std::endian::native == std::endian::big) { result[size - i - 1] = ptr[i]; }
        else {
            result[i] = ptr[i];
        }
    }
    return result;
}

static std::vector<uint8_t> serializeUnsigned(uint64_t value, std::size_t size)
{
    std::vector<uint8_t> result(size, 0);
    auto*                ptr = reinterpret_cast<uint8_t*>(&value);
    for (size_t i = 0; i < size; ++i) {
        // ReSharper disable once CppDFAUnreachableCode
        if constexpr (std::endian::native == std::endian::big) { result[size - i - 1] = ptr[i]; }
        else {
            result[i] = ptr[i];
        }
    }
    return result;
}

static std::vector<uint8_t> serializeTimeStruct(const sol::object& value)
{
    auto     table = value.as<sol::table>();
    uint32_t ms    = table["ms"].get<uint32_t>() & 0x0F'FF'FF'FF;
    uint16_t days  = table["days"].get<uint16_t>();

    auto result = serializeUnsigned(ms, 4);
    auto sDays  = serializeUnsigned(days, 2);
    result.insert(result.end(), sDays.begin(), sDays.end());
    return result;
}

std::vector<uint8_t> serializeOdeValue(const sol::table& ode, const sol::object& value)
{
    switch (static_cast<DataType>(ode["dataType"].get<uint16_t>())) {
        case DataType::boolean: return std::vector<uint8_t>(1, value.as<bool>() ? 1 : 0);
        case DataType::real32: return serializeFloat(value.as<float>());
        case DataType::real64: return serializeDouble(value.as<double>());

        case DataType::integer8: return serializeInteger(value.as<int64_t>(), 1);
        case DataType::integer16: return serializeInteger(value.as<int64_t>(), 2);
        case DataType::integer24: return serializeInteger(value.as<int64_t>(), 3);
        case DataType::integer32: return serializeInteger(value.as<int64_t>(), 4);
        case DataType::integer40: return serializeInteger(value.as<int64_t>(), 5);
        case DataType::integer48: return serializeInteger(value.as<int64_t>(), 6);
        case DataType::integer56: return serializeInteger(value.as<int64_t>(), 7);
        case DataType::integer64: return serializeInteger(value.as<int64_t>(), 8);
        case DataType::unsigned8: return serializeUnsigned(value.as<uint64_t>(), 1);
        case DataType::unsigned16: return serializeUnsigned(value.as<uint64_t>(), 2);
        case DataType::unsigned24: return serializeUnsigned(value.as<uint64_t>(), 3);
        case DataType::unsigned32: return serializeUnsigned(value.as<uint64_t>(), 4);
        case DataType::unsigned40: return serializeUnsigned(value.as<uint64_t>(), 5);
        case DataType::unsigned48: return serializeUnsigned(value.as<uint64_t>(), 6);
        case DataType::unsigned56: return serializeUnsigned(value.as<uint64_t>(), 7);
        case DataType::unsigned64: return serializeUnsigned(value.as<uint64_t>(), 8);

        case DataType::visibleString: {
            auto str = value.as<std::string>();
            auto fieldLen = ode["stringLengthMin"].get<uint32_t>();
            if (str.size() < fieldLen) { str.insert(str.end(), fieldLen - str.size(), 0); }
            return std::vector<uint8_t>(str.begin(), str.end());
        }
        case DataType::octetString: {
            auto str = value.as<std::string>();
            auto fieldLen = ode["stringLengthMin"].get<uint32_t>();
            if (str.size() < fieldLen) { str.insert(str.end(), fieldLen - str.size(), 0); }
            return std::vector<uint8_t>(str.begin(), str.end());
        }
        case DataType::unicodeString: {
            auto str = value.as<std::string>();
            auto fieldLen = ode["stringLengthMin"].get<uint32_t>();
            if (str.size() < fieldLen) { str.insert(str.end(), fieldLen - str.size(), 0); }
            return std::vector<uint8_t>(str.begin(), str.end());
        }
        case DataType::domain: return value.as<std::vector<uint8_t>>();

        case DataType::timeOfDay: return serializeTimeStruct(value);
        case DataType::timeDifference: return serializeTimeStruct(value);

        // not implemented
        case DataType::pdoCommunicationParameter:
        case DataType::pdoMapping:
        case DataType::sdoParameter:
        case DataType::identity:
        default: throw std::runtime_error("Not implemented");
    }
}
