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
#include "utils/models/sequence.h"
#include "utils/models/solution.h"

#include <barrier>
#include <functional>
#include <future>
#include <map>
#include <sol/sol.hpp>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

namespace Frasy::Lua
{
class Orchestrator
{
public:
    class Interface;

    enum class Stage
    {
        Idle       = 0,
        Generation = 1,
        Validation = 2,
        Execution  = 3,
    };
    static std::string stage2str(Stage stage);

public:
    static constexpr std::string_view passSubdirectory = "pass";
    static constexpr std::string_view failSubdirectory = "fail";
    static constexpr std::string_view lastSubdirectory = "last";
    static constexpr std::string_view solutionFile     = "lua/solution.json";

public:
    Orchestrator() = default;

    /**
     * Load the user environment files
     * @param environment path to the environment file
     * @param testDir path the folder containing the test files
     * @return true if the orchestrator successfully loaded the files, false otherwise
     */
    bool LoadUserFiles(const std::string& environment, const std::string& testsDir);

    /**
     * Async request to generate the solution
     * Success will be reported through Interface::OnGenerated() callback
     */
    void Generate();

    /**
     * Toggle the enable state of a uut
     * @param index the index of the UUT (start at 1)
     */
    void ToggleUut(std::size_t index);

    /**
     * Enable or disable a test from running
     * Use GetSolution() to know a test state
     * @param sequence name of the sequence that hold the test
     * @param test name of the test
     * @param enable state of the test
     */
    void SetTestEnable(const std::string& sequence, const std::string& test, bool enable);

    /**
     * Enable or disable a sequence from running
     * Use GetSolution() to know a sequence state
     * @param sequence name of the sequence
     * @param enable state of the sequence
     */
    void SetSequenceEnable(const std::string& sequence, bool enable);

    /**
     * Execute the solution asynchronously
     * Will generate if no solution was computed
     * Will validate then execute the solution at each run
     * Request will be ignored if a solution is already being run
     * Use IsRunning() to know if a solution is already being run
     * Use UutState() to know the state of the UUT
     * @param serials list of UUT serial, must match the number of UUTs
     * @param regenerate if true, will force the generation of the solution
     * @param skipVerification if true, will skip the validation step
     */
    void RunSolution(const std::vector<std::string>& serials, bool regenerate, bool skipVerification);

    /**
     * Return the calculated solution
     * @return
     */
    const Models::Solution& GetSolution();

    /**
     * Allows orchestrator to display Lua popups
     * Already call by Frasy::MainLayer::OnGuiRender()
     */
    void RenderPopups();

    const Map& GetMap() const { return m_map; }

    [[nodiscard]] bool     IsRunning() const;
    [[nodiscard]] UutState GetUutState(std::size_t uut) const;
    void                   SetPopulateUserMethodsCallback(std::function<void(sol::state_view, Stage)> callback);

private:
    bool        CreateOutputDirs();
    bool        InitLua(sol::state_view lua, std::size_t uut = 0, Stage stage = Stage::Generation);
    void        ImportExclusive(sol::state_view lua, Stage stage);
    static void ImportLog(sol::state_view lua, std::size_t uut, Stage stage);
    void        ImportPopup(sol::state_view lua, std::size_t uut, Stage stage);
    void        LoadIb(sol::state_view lua);
    static void LoadIbCommandForExecution(sol::state_view lua, const Frasy::Actions::CommandInfo::Reply& fun);
    static void LoadIbCommandForValidation(sol::state_view lua, const Frasy::Actions::CommandInfo::Reply& fun);
    static bool LoadEnvironment(sol::state_view lua, const std::string& filename);
    static bool LoadTests(sol::state_view lua, const std::string& filename);
    void        RunTests(const std::vector<std::string>& serials, bool regenerate, bool skipVerification);

    void PopulateMap();
    void UpdateUutState(enum UutState state, bool force = false);
    void UpdateUutState(enum UutState state, const std::vector<std::size_t>& uuts, bool force = false);

    bool RunStageGenerate(bool regenerate = false);
    bool RunStageVerify(sol::state_view team);
    void RunStageExecute(sol::state_view team, const std::vector<std::string>& serials);
    void CheckResults(const std::vector<std::size_t>& devices);

private:
    std::unique_ptr<sol::state>  m_state     = nullptr;
    std::vector<Frasy::UutState> m_uutStates = {};
    std::future<void>            m_running;
    Frasy::Map                   m_map;
    bool                         m_generated = false;
    std::string                  m_environment;
    std::string                  m_testsDir;
    std::string                  m_outputDirectory = "logs";
    bool                         m_ibEnabled       = true;

    std::map<std::string, Frasy::Lua::Popup*> m_popups     = {};
    std::unique_ptr<std::mutex>               m_popupMutex = nullptr;

    std::unique_ptr<std::mutex>       m_exclusiveLock    = nullptr;
    std::map<std::size_t, std::mutex> m_exclusiveLockMap = {};

    std::function<void(sol::state_view lua, Stage stage)> m_populateUserMethods = [](sol::state_view, Stage) {};
    Models::Solution                                      m_solution            = {};
};

}    // namespace Frasy::Lua

#endif    // BRIGERAD_FRASY_LUA_INTERPRETER_H
