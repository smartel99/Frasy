/**
 * @file    interpreter.h
 * @author  Paul Thomas
 * @date    2023-02-27
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
#ifndef BRIGERAD_FRASY_LUA_INTERPRETER_H
#define BRIGERAD_FRASY_LUA_INTERPRETER_H

#include "../../commands/type/struct.h"
#include "../../communication/serial/device.h"
#include "../../concepts.h"
#include "../../map.h"
#include "../../UutState.h"
#include "utils/lua/popup.h"

#include <barrier>
#include <functional>
#include <future>
#include <map>
#include <sol/sol.hpp>
#include <string>
#include <thread>
#include <unordered_map>

namespace Frasy::Lua
{

class Orchestrator
{
public:
    enum class Stage
    {
        Idle       = 0,
        Generation = 1,
        Validation = 2,
        Execution  = 3,
    };

public:
    static constexpr std::string_view passDirectory = "pass";
    static constexpr std::string_view failDirectory = "fail";
    static constexpr std::string_view lastDirectory = "last";

private:
    std::unique_ptr<sol::state> m_state;
    std::vector<UutState>       m_uutStates;
    std::future<void>           m_running;
    Frasy::Map                  m_map;
    bool                        m_generated;
    std::string                 m_environment;
    std::string                 m_testsDir;
    std::string                 m_outputDirectory = "logs";
    type_id_t                   m_testPointType   = 0;
    bool                        m_ibEnabled       = true;

    std::map<std::string, Frasy::Lua::Popup*> m_popups;
    std::unique_ptr<std::mutex>               m_popupMutex;

    std::unique_ptr<std::barrier<>> m_globalSync;

public:
    void Init(const std::string& environment, const std::string& testsDir);
    void RenderPopups();
    void DoTests(const std::vector<std::string>& serials, bool regenerate = true);
    Map  GetMap() const { return m_map; }
    void ToggleUut(std::size_t index);
    void EnableIb(bool enable = true) { m_ibEnabled = enable; }

    [[nodiscard]] bool     IsRunning() const;
    [[nodiscard]] UutState UutState(std::size_t uut) const;


private:
    bool CreateOutputDirs();
    void InitLua(sol::state& lua, std::size_t uut = 0, const std::string& state = "Generation");
    void ImportLog(sol::state& lua, std::size_t uut);
    void ImportPopup(sol::state& lua, std::size_t uut);
    void ImportSync(sol::state& lua);
    void LoadIb(sol::state& lua);
    void LoadIbCommandForExecution(sol::state& lua, const Frasy::Actions::CommandInfo::Reply& fun);
    void LoadIbCommandForValidation(sol::state& lua, const Frasy::Actions::CommandInfo::Reply& fun);
    bool LoadEnvironment(sol::state& lua, const std::string& filename);
    bool LoadTests(sol::state& lua, const std::string& filename);
    bool DoStep(sol::state& lua, const std::string& filename);
    void RunTests(const std::vector<std::string>& serials, bool regenerate);

    void PopulateMap();
    void UpdateUutState(enum UutState state, bool force = false);
    void UpdateUutState(enum UutState state, const std::vector<std::size_t>& uuts, bool force = false);

    bool Generate(bool regenerate = false);
    bool Verify(const sol::state& team);
    void Execute(const sol::state& team, const std::vector<std::string>& serials);
    void CheckResults(const std::vector<std::size_t>& devices);
};

}    // namespace Frasy::Lua

#endif    // BRIGERAD_FRASY_LUA_INTERPRETER_H
