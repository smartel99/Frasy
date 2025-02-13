/**
 * @file    orchestrator.h
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
 * not, see <https://www.gnu.org/licenses>
 */
#ifndef BRIGERAD_FRASY_LUA_INTERPRETER_H
#define BRIGERAD_FRASY_LUA_INTERPRETER_H

#include "../../commands/type/struct.h"
#include "../../communication/can_open/can_open.h"
#include "../../communication/serial/device.h"
#include "../../UutState.h"
#include "../map.h"
#include "utils/lua/popup.h"
#include "utils/models/solution.h"

#include <functional>
#include <future>
#include <map>
#include <sol/sol.hpp>
#include <string>

namespace Frasy::Lua {
class Orchestrator {
public:
    class Interface;

    enum class Stage {
        idle       = 0,
        generation = 1,
        validation = 2,
        execution  = 3,
    };
    static std::string stage2str(Stage stage);

    static constexpr std::string_view passSubdirectory = "pass";
    static constexpr std::string_view failSubdirectory = "fail";
    static constexpr std::string_view lastSubdirectory = "last";
    static constexpr std::string_view solutionFile     = "lua/solution.json";

    Orchestrator() = default;

    void setCanOpen(CanOpen::CanOpen* instance) { m_canOpen = instance; }

    /**
     * Load the user environment files
     * @param environment path to the environment file
     * @param testsDir path the folder containing the test files
     * @return true if the orchestrator successfully loaded the files, false otherwise
     */
    bool loadUserFiles(const std::string& environment, const std::string& testsDir);

    /**
     * Async request to generate the solution
     * Success will be reported through Interface::OnGenerated() callback
     */
    void generate();

    /**
     * Toggle the enable state of a uut
     * @param index the index of the UUT (start at 1)
     */
    void toggleUut(std::size_t index);

    /**
     * Force update the state of a UUT
     * Should not be used unless you are performing operation outside lua
     * @param index uut to update
     * @param state
     */
    void setUutState(std::size_t index, UutState state);

    /**
     * Enable or disable a test from running
     * Use GetSolution() to know a test state
     * @param sequence name of the sequence that hold the test
     * @param test name of the test
     * @param enable state of the test
     */
    void setTestEnable(const std::string& sequence, const std::string& test, bool enable);

    /**
     * Enable or disable a sequence from running
     * Use GetSolution() to know a sequence state
     * @param sequence name of the sequence
     * @param enable state of the sequence
     */
    void setSequenceEnable(const std::string& sequence, bool enable);

    /**
     * Execute the solution asynchronously
     * Will generate if no solution was computed
     * Will validate then execute the solution at each run
     * Request will be ignored if a solution is already being run
     * Use isRunning() to know if a solution is already being run
     * Use UutState() to know the state of the UUT
     * @param operatorName
     * @param serials list of UUT serial, must match the number of UUTs
     * @param regenerate if true, will force the generation of the solution
     * @param skipVerification if true, will skip the validation step
     */
    void runSolution(const std::string&              operatorName,
                     const std::vector<std::string>& serials,
                     bool                            regenerate,
                     bool                            skipVerification);

    /**
     * Return the calculated solution
     * @return
     */
    const Models::Solution& getSolution();

    /**
     * Allows orchestrator to display Lua popups
     * Already call by Frasy::MainLayer::OnGuiRender()
     */
    void renderPopups();

    [[nodiscard]] const Map& getMap() const { return m_map; }

    [[nodiscard]] bool        isRunning() const;
    [[nodiscard]] UutState    getUutState(std::size_t uut) const;
    void                      setLoadUserFunctions(const std::function<void(sol::state_view)>& callback);
    void                      setLoadUserBoards(const std::function<sol::table(sol::state_view)>& callback);
    void                      setLoadUserValues(const std::function<sol::table(sol::state_view)>& callback);
    [[nodiscard]] std::string getTitle() const { return m_title; }

private:
    bool        createOutputDirs();
    bool        initLua(sol::state_view lua, std::size_t uut = 0, Stage stage = Stage::generation);
    void        importExclusive(sol::state_view lua, Stage stage);
    static void importLog(sol::state_view lua, std::size_t uut, Stage stage);
    void        importPopup(sol::state_view lua, std::size_t uut, Stage stage);
    static bool loadEnvironment(sol::state_view lua, const std::string& filename);
    static bool loadTests(sol::state_view lua, const std::string& filename);
    void        runTests(const std::vector<std::string>& serials, bool regenerate, bool skipVerification);

    void populateMap();
    void updateUutState(UutState state, bool force = false);
    void updateUutState(UutState state, const std::vector<std::size_t>& uuts, bool force = false);

    bool runStageGenerate(bool regenerate = false);
    bool runStageVerify(sol::state_view team);
    void runStageExecute(sol::state_view team, const std::vector<std::string>& serials);
    void checkResults(const std::vector<std::size_t>& devices);

    std::unique_ptr<sol::state> m_state = nullptr;
    std::vector<UutState>       m_uutStates;
    std::future<void>           m_running;
    Map                         m_map;
    bool                        m_generated = false;
    std::string                 m_title;
    std::string                 m_environment;
    std::string                 m_testsDir;
    std::string                 m_outputDirectory = "logs";
    std::string                 m_operator;
    bool                        m_ibEnabled = true;

    std::map<std::string, Popup*> m_popups;
    std::unique_ptr<std::mutex>   m_popupMutex = nullptr;

    std::unique_ptr<std::mutex>       m_exclusiveLock = nullptr;
    std::map<std::size_t, std::mutex> m_exclusiveLockMap;

    std::function<void(sol::state_view lua)>       m_loadUserFunctions = [](sol::state_view lua) {};
    std::function<sol::table(sol::state_view lua)> m_loadUserBoards    = [](sol::state_view lua) {
        return lua.create_table();
    };
    std::function<sol::table(sol::state_view lua)> m_loadUserValues = [](sol::state_view lua) {
        // Values will be available at Context.values.gui
        return lua.create_table();
    };
    Models::Solution m_solution = {};

    CanOpen::CanOpen* m_canOpen = nullptr;
};

}    // namespace Frasy::Lua

#endif    // BRIGERAD_FRASY_LUA_INTERPRETER_H
