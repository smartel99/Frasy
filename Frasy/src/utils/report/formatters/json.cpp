/**
 * @file    json.cpp
 * @author  Paul Thomas
 * @date    3/17/2025
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
 * not, see <https://www.gnu.org/licenses/>.
 */

#include "json.h"
#include <filesystem>
#include <fstream>
#include <utils/lua/json_converter.h>
#include <utils/report/utils/obj2str.h>

namespace Frasy::Report::Formatter {

Json::Json(const sol::table& result) : Formatter(result)
{
    m_object["ib"]        = nlohmann::ordered_json::object();
    m_object["info"]      = nlohmann::ordered_json::object();
    m_object["sequences"] = nlohmann::ordered_json::object();
}

void Json::reportInfo()
{
    auto&       info    = m_object["info"];
    const auto& section = m_result["info"];
    info["date"]        = section["date"].get<std::string>();
    info["operator"]    = section["operator"].get<std::string>().c_str();
    info["pass"]        = section["pass"].get<bool>();
    info["serial"]      = section["serial"].get<std::string>().c_str();
    info["time"]        = reportSectionTime(section);
    info["title"]       = section["title"].get<std::string>();
    info["uut"]         = section["uut"].get<int>();
    info["version"]     = nlohmann::ordered_json::object();
    for (const auto& field : {"frasy", "orchestrator", "scripts", "application"}) {
        info["version"][field] = section["version"][field].get<std::string>();
    }
}

void Json::reportUserInfo(const sol::table& table)
{
    auto& info = m_object["info"];
    for (const auto& [keyObj, value] : table) {
        const auto key = obj2str(keyObj);
        switch (value.get_type()) {
            case sol::type::none: info[key] = "none"; break;
            case sol::type::nil: info[key] = "nil"; break;
            case sol::type::string: info[key] = value.as<std::string>(); break;
            case sol::type::number: info[key] = value.as<double>(); break;
            case sol::type::thread: info[key] = fmt::format("thread: {}", value.pointer()); break;
            case sol::type::boolean: info[key] = value.as<bool>(); break;
            case sol::type::function: info[key] = fmt::format("function: {}", value.pointer()); break;
            case sol::type::userdata: info[key] = fmt::format("userdata: {}", value.pointer()); break;
            case sol::type::lightuserdata: info[key] = fmt::format("lightuserdata: {}", value.pointer()); break;
            case sol::type::table: info[key] = fmt::format("table: {}", value.pointer()); break;
            case sol::type::poly: info[key] = fmt::format("poly: {}", value.pointer()); break;
        }
    }
}

void Json::reportIb(const std::string& name)
{
    auto ib              = nlohmann::ordered_json::object();
    ib["hardware"]       = m_result["ib"][name]["hardware"].get<std::string>();
    ib["nodeId"]         = m_result["ib"][name]["nodeId"].get<int>();
    ib["serial"]         = m_result["ib"][name]["serial"].get<std::string>();
    ib["software"]       = m_result["ib"][name]["software"].get<std::string>();
    m_object["ib"][name] = ib;
}

void Json::reportSequenceResult(const std::string& name)
{
    setSequence(name);
    auto obj  = nlohmann::ordered_json::object();
    m_section = &obj;
    reportSectionBaseResult(m_result["sequences"][name].get_or(m_emptyTable));
    m_section                   = nullptr;
    obj["tests"]                = nlohmann::ordered_json::object();
    m_object["sequences"][name] = obj;
}

void Json::reportTestResult(const std::string& name)
{
    setTest(name);
    auto obj  = nlohmann::ordered_json::object();
    m_section = &obj;
    reportSectionBaseResult(m_result["sequences"][m_sequenceName]["tests"].get_or(m_emptyTable)[name].get_or(m_emptyTable));
    m_section                                            = nullptr;
    obj["expectations"]                                  = nlohmann::ordered_json::array();
    m_object["sequences"][m_sequenceName]["tests"][name] = obj;
}

void Json::toFile(const std::string& filename)
{
    std::ofstream ofs(filename, std::ios::out | std::ios::trunc);
    ofs << std::setw(4) << m_object << std::endl;    // setw to prettify
}

void Json::reportExpectation(const sol::table& expectation)
{
    auto&      expectations = m_object["sequences"][m_sequenceName]["tests"][m_testName]["expectations"];
    const auto jEx          = Lua::tableToJson(expectation);
    expectations.push_back(nlohmann::ordered_json(jEx)); // We can ignore order here, expectations are in array
}

void Json::reportSectionBaseResult(const sol::table& section) const
{
    auto& obj      = *m_section;
    obj["enabled"] = section["enabled"].get_or(false);
    obj["pass"]    = section["pass"].get_or(false);
    obj["skipped"] = section["skipped"].get_or(true);
    obj["time"]    = reportSectionTime(section);
}

nlohmann::ordered_json Json::reportSectionTime(const sol::table& section)
{
    auto time = nlohmann::ordered_json::object();
    for (const auto& field : {"elapsed", "process", "start", "stop"}) {
        time[field] = section["time"][field].get_or(0.0);
    }
    return time;
}


}    // namespace Frasy::Report::Formatter