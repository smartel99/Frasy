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
    static std::string stage2str(Stage stage);

public:
    static constexpr std::string_view passDirectory = "pass";
    static constexpr std::string_view failDirectory = "fail";
    static constexpr std::string_view lastDirectory = "last";

private:
    std::unique_ptr<sol::state> m_state = nullptr;
    std::vector<UutState>       m_uutStates = {};
    std::future<void>           m_running;
    Frasy::Map                  m_map;
    bool                        m_generated = false;
    std::string                 m_environment;
    std::string                 m_testsDir;
    std::string                 m_outputDirectory = "logs";
    type_id_t                   m_testPointType   = 0;
    bool                        m_ibEnabled       = true;

    std::map<std::string, Frasy::Lua::Popup*> m_popups = {};
    std::unique_ptr<std::mutex>               m_popupMutex = nullptr;

    std::unique_ptr<std::barrier<>> m_globalSync = nullptr;

    std::unique_ptr<std::mutex>       m_exclusiveLock = nullptr;
    std::map<std::size_t, std::mutex> m_exclusiveLockMap = {};

    std::function<void(sol::state& lua, Stage stage)> m_populateUserMethods = [](sol::state&, Stage) {};

public:
    bool       Init(const std::string& environment, const std::string& tests);
    void       RenderPopups();
    void       DoTests(const std::vector<std::string>& serials, bool regenerate, bool skipVerification);
    const Map& GetMap() const { return m_map; }
    void       ToggleUut(std::size_t index);
    void       EnableIb(bool enable = true) { m_ibEnabled = enable; }

    [[nodiscard]] bool     IsRunning() const;
    [[nodiscard]] UutState UutState(std::size_t uut) const;
    void                   SetPopulateUserMethodsCallback(std::function<void(sol::state&, Stage)> callback)
    {
        m_populateUserMethods = callback;
    }

private:
    bool        CreateOutputDirs();
    bool        InitLua(sol::state& lua, std::size_t uut = 0, Stage stage = Stage::Generation);
    void        ImportExclusive(sol::state& lua, Stage stage);
    static void        ImportLog(sol::state& lua, std::size_t uut, Stage stage);
    void        ImportPopup(sol::state& lua, std::size_t uut, Stage stage);
    void        ImportSync(sol::state& lua, Stage stage);
    void        LoadIb(sol::state& lua);
    static void        LoadIbCommandForExecution(sol::state& lua, const Frasy::Actions::CommandInfo::Reply& fun);
    static void        LoadIbCommandForValidation(sol::state& lua, const Frasy::Actions::CommandInfo::Reply& fun);
    static bool LoadEnvironment(sol::state& lua, const std::string& filename);
    static bool LoadTests(sol::state& lua, const std::string& filename);
    bool        DoStep(sol::state& lua, const std::string& filename);
    void               RunTests(const std::vector<std::string>& serials, bool regenerate, bool skipVerification);

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
