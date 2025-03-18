/**
 * @file    frasy_interpreter.cpp
 * @author  Samuel Martel
 * @date    2022-12-13
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
#include "frasy_interpreter.h"

#include "utils/communication/serial/enumerator.h"

#include <serial/serial.h>

namespace Frasy {
namespace {
constexpr auto configFilepath = "config.json";
}

nlohmann::json Interpreter::loadConfig()
{
    if (!std::filesystem::exists(configFilepath)) { return nlohmann::json::object(); }
    nlohmann::json json;
    try {
        std::ifstream ifs(configFilepath);
        try {
            ifs >> json;
            if (!json.is_object()) { throw std::runtime_error("JSON is not an object"); }
        }
        catch (const std::exception& e) {
            BR_APP_ERROR("Failed to parse configuration file. Reason: {}", e.what());
            json = nlohmann::json::object();
        }
        ifs.close();
    }
    catch (const std::exception& e) {
        BR_APP_ERROR("Failure during IO operation on config file. Reason: {}", e.what());
        json = nlohmann::json::object();
    }
    return json;
}

void Interpreter::saveConfig() const
{
    if (std::filesystem::exists(configFilepath)) { std::filesystem::remove(configFilepath); }
    try {
        std::ofstream ofs(configFilepath, std::ios::out | std::ios::binary | std::ios::trunc);
        try {
            ofs << std::setw(4) << m_config << std::endl;
        }
        catch (const std::exception& e) {
            BR_APP_ERROR("Failed to write config to file. Reason: {}", e.what());
        }
        ofs.close();
    }
    catch (const std::exception& e) {
        BR_APP_ERROR("Failure during IO operation on config file. Reason: {}", e.what());
    }
}



}    // namespace Frasy
