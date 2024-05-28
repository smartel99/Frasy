/**
 * @file    sdo_deserializer.cpp
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

#include "ode_deserializer.h"
#include "../communication/can_open/types.h"
#include "utils/misc/deserializer.h"

using DataType = Frasy::CanOpen::DataType;

static int64_t deserializeInteger(const std::span<uint8_t>& value)
{
    int64_t result = 0;
    auto*   ptr    = reinterpret_cast<uint8_t*>(&result);
    for (int i = 0; i < value.size(); ++i) {
        ptr[i] = value[i];
    }
    result <<= 64 - (value.size() * 8);
    result >>= 64 - (value.size() * 8);
    return result;
}

static uint64_t deserializeUnsigned(const std::span<uint8_t>& value)
{
    uint64_t result = 0;
    auto*    ptr    = reinterpret_cast<uint8_t*>(&result);
    for (int i = 0; i < value.size(); ++i) {
        ptr[i] = value[i];
    }
    return result;
}

static sol::object deserializeTimeStruct(sol::state_view& lua, const std::span<uint8_t>& value)
{
    auto     table = lua.create_table();
    uint32_t ms    = deserializeUnsigned(value.subspan(0, 4));
    table["ms"]    = 0x0F'FF'FF'FF & ms;    // u28, not u24
    table["days"]  = deserializeUnsigned(value.subspan(4, 2));
    return table;
}

sol::object deserializeOdeValue(sol::state_view& lua, const sol::table& ode, const std::span<uint8_t>& value)
{
    switch (static_cast<DataType>(ode["dataType"].get<uint16_t>())) {
        case DataType::boolean: return make_object(lua, value[0] != 0);
        case DataType::real32: return make_object(lua, Frasy::Deserialize<float>(value.begin(), value.end()));
        case DataType::real64: return make_object(lua, Frasy::Deserialize<double>(value.begin(), value.end()));

        case DataType::integer8: return make_object(lua, deserializeInteger(value));
        case DataType::integer16: return make_object(lua, deserializeInteger(value));
        case DataType::integer32: return make_object(lua, deserializeInteger(value));
        case DataType::integer64: return make_object(lua, deserializeInteger(value));
        case DataType::unsigned8: return make_object(lua, deserializeUnsigned(value));
        case DataType::unsigned16: return make_object(lua, deserializeUnsigned(value));
        case DataType::unsigned32: return make_object(lua, deserializeUnsigned(value));
        case DataType::unsigned64: return make_object(lua, deserializeUnsigned(value));
        case DataType::integer24: return make_object(lua, deserializeInteger(value));
        case DataType::integer40: return make_object(lua, deserializeInteger(value));
        case DataType::integer48: return make_object(lua, deserializeInteger(value));
        case DataType::integer56: return make_object(lua, deserializeInteger(value));
        case DataType::unsigned24: return make_object(lua, deserializeUnsigned(value));
        case DataType::unsigned40: return make_object(lua, deserializeUnsigned(value));
        case DataType::unsigned48: return make_object(lua, deserializeUnsigned(value));
        case DataType::unsigned56: return make_object(lua, deserializeUnsigned(value));

        case DataType::visibleString: return make_object(lua, std::string(value.begin(), value.end()));
        case DataType::octetString: return make_object(lua, std::string(value.begin(), value.end()));
        case DataType::unicodeString: return make_object(lua, std::wstring(value.begin(), value.end()));
        case DataType::domain: return make_object(lua, value);

        // PFR eligible
        case DataType::timeOfDay: return deserializeTimeStruct(lua, value);
        case DataType::timeDifference:
            return deserializeTimeStruct(lua, value);

            // not implemented
        case DataType::pdoCommunicationParameter:
        case DataType::pdoMapping:
        case DataType::sdoParameter:
        case DataType::identity:
        default: throw std::runtime_error("Not implemented");
    }
}
