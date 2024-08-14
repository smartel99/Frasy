/**
 * @file    interpreter.cpp
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

#include "orchestrator.h"

#include "../../commands/built_in/status/reply.h"
#include "../../communication/can_open/services/sdo.h"
#include "../../communication/serial/device_map.h"
#include "../args_checker.h"
#include "../dummy_table_deserializer.h"
#include "../ode_deserializer.h"
#include "../ode_serializer.h"
#include "../table_deserializer.h"
#include "../table_serializer.h"
#include "../tag.h"
#include "../team.h"
#include "utils/commands/type/manager/manager.h"
#include "utils/lua/save_as_json.h"
#include "utils/lua/version.h"
#include "utils/misc/serializer.h"

#include <Brigerad/Utils/dialogs/warning.h>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <json.hpp>
#include <stdexcept>

#include <processthreadsapi.h>

namespace Frasy::Lua {

// <editor-fold desc="Orchestrator">
std::string Orchestrator::stage2str(Stage stage)
{
    switch (stage) {
        case Stage::generation: return "generation";
        case Stage::validation: return "validation";
        case Stage::execution: return "execution";
        case Stage::idle:
        default: return "Idle";
    }
}

// <editor-fold desc="Test related">
namespace {
int OnPanic(lua_State* lua)
{
    FRASY_PROFILE_FUNCTION();
    // Get the error message from the top of the stack.
    std::string msg;
    if (lua_isstring(lua, -1)) { msg = lua_tostring(lua, -1); }
    else {
        msg = "[No message provided]";
    }
    BR_LUA_CRITICAL("An error occurred:");
    BR_LUA_ERROR("{}\n\r", msg);
    luaL_traceback(lua, lua, nullptr, 0);
    msg = lua_tostring(lua, -1);
    BR_LUA_ERROR("{}\n\r", msg);
    //    luaL_dostring(lua, "Log.Error(debug.traceback())");

    return 0;
}
}    // namespace


bool Orchestrator::loadUserFiles(const std::string& environment, const std::string& testsDir)
{
    if (FAILED(SetThreadDescription(GetCurrentThread(), L"Lua File Loader"))) {
        BR_LOG_ERROR("Orchestrator", "Unable to set thread name");
    }
    FRASY_PROFILE_FUNCTION();
    m_popupMutex = std::make_unique<std::mutex>();
    m_state      = std::make_unique<sol::state>();
    m_state->set_panic(&OnPanic);
    m_map         = {};
    m_generated   = false;
    m_environment = environment;
    m_testsDir    = testsDir;
    InitLua(*m_state);
    if (!LoadEnvironment(*m_state, m_environment)) { return false; }
    if (!LoadTests(*m_state, m_testsDir)) { return false; }
    PopulateMap();
    m_uutStates.resize(m_map.uuts.size() + 1, UutState::Idle);

    return true;
}
// </editor-fold>


void Orchestrator::RunSolution(const std::vector<std::string>& serials, bool regenerate, bool skipVerification)
{
    if (isRunning()) {
        Brigerad::warningDialog("Frasy", "Test is already running!");
        return;
    }
    if (m_map.uuts.empty()) {
        Brigerad::warningDialog("Frasy", "No UUTs to test!");
        return;
    }
    m_running = std::async(std::launch::async, [this, &serials, regenerate, skipVerification] {
        if (FAILED(SetThreadDescription(GetCurrentThread(), L"Solution Runner"))) {
            BR_LOG_ERROR("Orchestrator", "Unable to set thread name");
        }
        FRASY_PROFILE_FUNCTION();
        RunTests(serials, regenerate, skipVerification);
    });
}

const Models::Solution& Orchestrator::GetSolution()
{
    return m_solution;
}

bool Orchestrator::CreateOutputDirs()
{
    FRASY_PROFILE_FUNCTION();
    namespace fs = std::filesystem;
    try {
        fs::create_directory(m_outputDirectory);
        fs::create_directory(std::format("{}/{}", m_outputDirectory, passSubdirectory));
        fs::create_directory(std::format("{}/{}", m_outputDirectory, failSubdirectory));

        const auto last = std::format("{}/{}", m_outputDirectory, lastSubdirectory);
        fs::remove_all(last);
        fs::create_directory(last);
    }
    catch (const fs::filesystem_error& e) {
        BR_LUA_ERROR(e.what());
        return false;
    }
    return true;
}

bool Orchestrator::InitLua(sol::state_view lua, std::size_t uut, Stage stage)
{
    FRASY_PROFILE_FUNCTION();
    try {
        lua.open_libraries(sol::lib::debug,
                           sol::lib::base,
                           sol::lib::table,
                           sol::lib::io,
                           sol::lib::package,
                           sol::lib::string,
                           sol::lib::math,
                           sol::lib::os);

        // Enums
        lua.script_file("lua/core/framework/stage.lua");

        // Variables
        lua.script_file("lua/core/framework/context.lua");

        lua["Context"]["info"]["stage"]   = lua["Stage"][stage2str(stage)];
        lua["Context"]["info"]["uut"]     = uut;
        lua["Context"]["info"]["version"] = "0.1.0";
        std::atomic_thread_fence(std::memory_order_release);
        lua["Context"]["values"]["gui"] = m_loadUserValues(lua);

        // Utils
        lua.require_file("Utils", "lua/core/utils/module.lua");
        lua["Utils"]["DirList"] = [](const std::string& path) {
            FRASY_PROFILE_FUNCTION();
            std::vector<std::string> files {};
            std::list<std::string>   directories {};
            directories.push_back(path);
            for (const auto& dir : directories) {
                for (const auto& entry : std::filesystem::directory_iterator {dir}) {
                    if (entry.is_directory()) {
                        directories.push_back(entry.path().string());
                        continue;
                    }
                    auto file = entry.path();
                    if (file.extension() == ".lua") {
                        file.replace_extension();
                        files.push_back(file.string());
                    }
                }
            }
            return sol::as_table(files);
        };
        lua["Utils"]["SleepFor"] =
            stage == Stage::execution ? [](int duration) {
                FRASY_PROFILE_FUNCTION();
                std::this_thread::sleep_for(std::chrono::milliseconds(duration)); }
            : [](int duration) { FRASY_PROFILE_FUNCTION(); };
        lua["Utils"]["SaveAsJson"] = [](sol::table table, const std::string& file) {
            FRASY_PROFILE_FUNCTION();
            SaveAsJson(table, file);
        };
        lua.require_file("Json", "lua/core/vendor/json.lua");
        ImportLog(lua, uut, stage);
        ImportPopup(lua, uut, stage);
        ImportExclusive(lua, stage);
        lua.script_file("lua/core/framework/exception.lua");

        // Framework
        lua.script_file("lua/core/sdk/environment/team.lua");
        lua.script_file("lua/core/sdk/environment/environment.lua");
        lua.script_file("lua/core/framework/orchestrator.lua");
        lua.script_file("lua/core/sdk/test.lua");

        // Communication
        lua.script_file("lua/core/can_open/can_open.lua");

        auto getIndexAndSubIndex = [](const sol::table& ode) {
            FRASY_PROFILE_FUNCTION();
            int index = 0;
            try {
                index = std::stoi(ode["index"].get<std::string>(), nullptr, 16);
            }
            catch (std::exception& e) {
                throw sol::error(std::format("Error when reading index: {}", e.what()));
            }

            int subIndex = 0;
            try {
                subIndex = std::stoi(ode["subIndex"].get<std::string>(), nullptr, 16);
            }
            catch (std::exception& e) {
                throw sol::error(std::format("Error when reading subIndex: {}", e.what()));
            }

            return std::make_pair(index, subIndex);
        };

        lua["CanOpen"]["__upload"] =
          [this, &getIndexAndSubIndex](sol::this_state state, std::size_t nodeId, const sol::table& ode) {
              FRASY_PROFILE_FUNCTION();
              sol::state_view lua  = sol::state_view(state.lua_state());
              auto*           node = m_canOpen->getNode(nodeId);
              if (node == nullptr) { throw sol::error("Invalid node id"); }
              auto* interface        = node->sdoInterface();
              auto [index, subIndex] = getIndexAndSubIndex(ode);
              {
                  FRASY_PROFILE_SCOPE("Upldoad loop");
                  auto request = interface->uploadData(index, subIndex, 100);
                  request.future.wait();
                  if (request.status() != CanOpen::SdoRequestStatus::Complete &&
                      request.status() != CanOpen::SdoRequestStatus::Cancelled) {
                      throw sol::error(std::format("Request failed: {}", request.status()));
                  }
                  auto result = request.future.get();
                  if (!result.has_value()) {
                      throw sol::error(std::format("Request failed with code {}: {}\nExtra: {}",
                                                   static_cast<int>(result.error()),
                                                   result.error(),
                                                   request.abortCode()));
                  }
                  auto value = deserializeOdeValue(lua, ode, result.value());
                  return value;
              }
          };

        lua["CanOpen"]["__download"] = [&](std::size_t nodeId, const sol::table& ode, sol::object value) {
            FRASY_PROFILE_FUNCTION();
            auto* node = m_canOpen->getNode(nodeId);
            if (node == nullptr) { throw sol::error(std::format("Node '{}' not found!", nodeId)); }
            auto* interface        = node->sdoInterface();
            auto [index, subIndex] = getIndexAndSubIndex(ode);
            auto sValue            = serializeOdeValue(ode, value);
            {
                FRASY_PROFILE_SCOPE("Download Loop");
                auto request = interface->downloadData(index, subIndex, sValue, 100);
                request.future.wait();
                if (request.status() != CanOpen::SdoRequestStatus::Complete &&
                    request.status() != CanOpen::SdoRequestStatus::Cancelled) {
                    throw sol::error(std::format("Request failed: {}", request.status()));
                }
                auto result = request.future.get();
                if (result != CO_SDO_RT_ok_communicationEnd) {
                    throw sol::error(std::format("Request failed with code {}: {}\nExtra: {}",
                                                 static_cast<int>(result),
                                                 result,
                                                 request.abortCode()));
                }
            }
        };

        // User functions
        m_loadUserFunctions(lua);

        // Boards
        auto ibs     = lua.create_named_table("Ibs");
        auto cepIbs  = lua.script_file("lua/core/cep/ibs.lua");
        auto userIbs = m_loadUserBoards(lua);
        for (auto& [k, v] : cepIbs.get<sol::table>()) {
            ibs[k] = v;
        }
        for (auto& [k, v] : userIbs) {
            ibs[k] = v;
        }

        // Profiling functions.
        lua["__profileStartEvent"] = sol::overload(
          [](sol::this_state state, const std::string& name) {
              if (name.empty()) { throw sol::error("Name cannot be empty!"); }
              lua_Debug ar {};
              lua_getstack(state.lua_state(), 1, &ar);
              lua_getinfo(state.lua_state(), "nSl", &ar);
              Profiler::get().reportCallEvent({name, ar.source, ar.currentline});
          },
          [](const std::string& name, const std::string& source, int line) {
              if (name.empty()) { throw sol::error("Name cannot be empty!"); }
              if (source.empty()) { throw sol::error("Source cannot be empty!"); }
              Profiler::get().reportCallEvent({name, source, line});
          });

        lua["__profileEndEvent"] = sol::overload(
          [](sol::this_state state, const std::string& name) {
              if (name.empty()) { throw sol::error("Name cannot be empty!"); }
              lua_Debug ar {};
              lua_getstack(state.lua_state(), 2, &ar);
              lua_getinfo(state.lua_state(), "nSl", &ar);
              Profiler::get().reportReturnEvent({name, ar.source, ar.currentline});
          },
          [](const std::string& name, const std::string& source, int line) {
              if (name.empty()) { throw sol::error("Name cannot be empty!"); }
              if (source.empty()) { throw sol::error("Source cannot be empty!"); }
              Profiler::get().reportReturnEvent({name, source, line});
          });

        lua_sethook(
          lua.lua_state(),
          [](lua_State* state, lua_Debug* ar) {
              lua_getinfo(state, "nSl", ar);
              std::string name = std::format("{}", ar->name == nullptr ? "<unknown>" : ar->name, ar->namewhat);

              if (name == "__profileStartEvent" || name == "__profileEndEvent") { return; }
              if (ar->source == nullptr) { ar->source = &ar->short_src[0]; }
              if (ar->event == LUA_HOOKCALL) {
                  Profiler::get().reportCallEvent({std::move(name), ar->source, ar->currentline});
              }
              else if (ar->event == LUA_HOOKRET) {
                  Profiler::get().reportReturnEvent({std::move(name), ar->source, ar->currentline});
              }
          },
          LUA_MASKCALL | LUA_MASKRET,
          0);

        return true;
    }
    catch (std::exception& e) {
        BR_LUA_ERROR("An error occurred while loading the framework: {}", e.what());
        return false;
    }
}

bool Orchestrator::LoadEnvironment(sol::state_view lua, const std::string& filename)
{
    sol::protected_function run = lua.script_file("lua/core/helper/load_environment.lua");
    run.error_handler           = lua.script_file("lua/core/framework/error_handler.lua");
    auto result                 = run(filename);
    if (!result.valid()) {
        sol::error err = result;
        lua["Log"]["E"](err.what());
    }
    else {
        lua["Log"]["I"]("Environment loaded successfully");
    }
    return result.valid();
}

bool Orchestrator::LoadTests(sol::state_view lua, const std::string& filename)
{
    FRASY_PROFILE_FUNCTION();
    sol::protected_function run = lua.script_file("lua/core/helper/load_tests.lua");
    run.error_handler           = lua.script_file("lua/core/framework/error_handler.lua");
    auto result                 = run(filename);
    if (!result.valid()) {
        sol::error err = result;
        lua["Log"]["E"](err.what());
    }
    else {
        lua["Log"]["I"]("Tests loaded successfully");
    }
    return result.valid();
}

void Orchestrator::RunTests(const std::vector<std::string>& serials, bool regenerate, bool skipVerification)
{
    UpdateUutState(UutState::Waiting);
    {
        FRASY_PROFILE_SCOPE("Create Output Dir");
        if (!CreateOutputDirs()) {
            BR_LUA_ERROR("Failed to create logs directories");
            UpdateUutState(UutState::Idle);
            return;
        }
    }
    {
        FRASY_PROFILE_SCOPE("Generation");
        if (!RunStageGenerate(regenerate)) {
            BR_LUA_ERROR("Generation failed");
            UpdateUutState(UutState::Error);
            return;
        }
    }
    {
        FRASY_PROFILE_SCOPE("Verify");
        if (!skipVerification && !RunStageVerify(*m_state)) {
            BR_LUA_ERROR("Verification failed");
            UpdateUutState(UutState::Error);
            return;
        }
    }
    {
        FRASY_PROFILE_SCOPE("Execute");
        RunStageExecute(*m_state, serials);
    }
}

bool Orchestrator::RunStageGenerate(bool regenerate)
{
    FRASY_PROFILE_FUNCTION();
    if (m_generated && !regenerate) { return true; }

    sol::state lua;
    InitLua(lua, 1);
    // LoadIb(lua);
    LoadEnvironment(lua, m_environment);
    LoadTests(lua, m_testsDir);
    sol::protected_function run = lua.script_file("lua/core/helper/generate.lua");
    run.error_handler           = lua.script_file("lua/core/framework/error_handler.lua");
    auto result                 = run(solutionFile);
    if (!result.valid()) {
        sol::error err = result;
        lua["Log"]["E"](err.what());
    }
    else {
        lua["Log"]["I"]("Success");
    }
    if (result.valid()) {
        using json               = nlohmann::json;
        std::ifstream os         = std::ifstream(std::string(solutionFile));
        json          generation = json::parse(os);
        m_solution.Clear();

        // Solution
        // - Section
        // - - Section stage
        // - - - Sequence (Actually a subsequence)
        // - - - - Sequence name
        // - - - - Sequence stage
        // - - - - - Test

        m_solution.sections.reserve(generation.size());
        for (const auto& gSc : generation) {
            // Section
            m_solution.sections.emplace_back();
            auto& sc = m_solution.sections.back();
            sc.reserve(gSc.size());
            for (const auto& gScS : gSc) {
                // Section stage
                sc.emplace_back();
                auto& scs = sc.back();
                scs.reserve(gScS.size());
                for (const auto& gSq : gScS) {
                    // Sequence
                    scs.emplace_back();
                    auto& sq = scs.back();
                    sq.first = gSq["name"];
                    sq.second.reserve(gSq["tests"].size());
                    if (!m_solution.sequences.contains(sq.first)) { m_solution.sequences[sq.first] = {}; }
                    for (const auto& gSqS : gSq["tests"]) {
                        // Sequence stages
                        sq.second.emplace_back();
                        auto& sqs = sq.second.back();
                        sqs.reserve(gSqS.size());
                        for (const auto& gT : gSqS) {
                            sqs.push_back(gT);
                            m_solution.sequences[sq.first].tests[gT] = {};
                        }
                    }
                }
            }
        }
    }

    m_generated = result.valid();
    return m_generated;
}

bool Orchestrator::RunStageVerify(sol::state_view team)
{
    FRASY_PROFILE_FUNCTION();
    bool hasTeam = team["Team"]["HasTeam"]();
    auto stages  = team["Context"]["worker"]["stages"].get<std::vector<sol::object>>();

    for (sol::object& stage : stages) {
        std::map<std::size_t, Team> teams;
        auto                        devices = stage.as<std::vector<std::size_t>>();
        if (hasTeam) {
            for (auto& uut : devices) {
                std::size_t leader      = team["Context"]["team"]["players"][uut]["leader"];
                auto        teamPlayers = team["Context"]["team"]["teams"][leader].get<std::vector<std::size_t>>();
                if (leader == uut) { teams[leader] = Team(teamPlayers.size()); }
            }
        }
        std::vector<std::thread>    threads;
        std::mutex                  mutex;
        std::map<std::size_t, bool> results;
        for (auto& uut : devices) {
            if (m_uutStates[uut] == UutState::Disabled) { continue; }
            threads.emplace_back([&, uut, team] {
                sol::state lua;
                InitLua(lua, uut, Stage::validation);
                // LoadIb(lua);
                LoadEnvironment(lua, m_environment);
                LoadTests(lua, m_testsDir);
                if (hasTeam) {
                    std::lock_guard lock {mutex};
                    int             leader   = team["Context"]["team"]["players"][uut]["leader"];
                    int             position = team["Context"]["team"]["players"][uut]["position"];
                    teams[leader].InitializeState(lua, uut, position, uut == leader);
                }
                sol::protected_function load_solution =
                  lua.script("return function(fp) Orchestrator.LoadSolution(fp) end");
                load_solution.error_handler = lua.script_file("lua/core/framework/error_handler.lua");
                auto rls                    = load_solution(solutionFile);
                if (!rls.valid()) {
                    sol::error err = rls;
                    lua["Log"]["e"](err.what());
                    std::lock_guard lock {mutex};
                    results[uut] = false;
                    return;
                }

                sol::protected_function verify = lua.script("return function() Orchestrator.Validate() end");
                verify.error_handler           = lua.script_file("lua/core/framework/error_handler.lua");
                auto rv                        = verify();
                if (!rv.valid()) {
                    sol::error err = rv;
                    lua["Log"]["e"](err.what());
                    std::lock_guard lock {mutex};
                    results[uut] = false;
                    return;
                }

                std::lock_guard lock {mutex};
                results[uut] = true;
                return;
            });
        }
        for (auto& thread : threads) {
            thread.join();
        }
        size_t expectedResults =
          std::accumulate(devices.begin(), devices.end(), size_t(0), [&](size_t tot, const auto& uut) {
              return tot + (m_uutStates[uut] == UutState::Disabled ? 0 : 1);
          });
        if (results.size() != expectedResults) {
            BR_LUA_ERROR("Missing results from validation");
            return false;
        }
        if (std::any_of(results.begin(), results.end(), [](const auto entry) { return !entry.second; })) {
            BR_LUA_ERROR("Not all UUT passed validation");
            return false;
        }
    }
    return true;
}

void Orchestrator::RunStageExecute(sol::state_view team, const std::vector<std::string>& serials)
{
    FRASY_PROFILE_FUNCTION();
    bool        hasTeam  = team["Team"]["HasTeam"]();
    auto        stages   = team["Context"]["worker"]["stages"].get<std::vector<sol::object>>();
    std::size_t uutCount = team["Context"]["map"]["uuts"].get<sol::table>().size();

    std::map<std::size_t, Team>       teams;
    std::map<std::size_t, sol::state> states;
    std::mutex                        mutex;
    std::map<std::size_t, bool>       results;

    for (std::size_t uut = 0; uut <= uutCount; ++uut) {
        states[uut] = sol::state();
    }

    auto hasCrashed = [&]() {
        FRASY_PROFILE_FUNCTION();
        if (std::any_of(results.begin(), results.end(), [](const auto& kvp) { return !kvp.second; })) {
            for (std::size_t uut = 1; uut <= uutCount; ++uut) {
                UpdateUutState(results[uut] ? UutState::Idle : UutState::Error, {uut}, false);
            }
            return true;
        }
        return false;
    };

    // Initialize lua states and teams
    for (sol::object& stage : stages) {
        auto devices = stage.as<std::vector<std::size_t>>();
        UpdateUutState(UutState::Running, devices);
        if (hasTeam) {
            for (auto& uut : devices) {
                std::size_t leader      = team["Context"]["team"]["players"][uut]["leader"];
                auto        teamPlayers = team["Context"]["team"]["teams"][leader].get<std::vector<std::size_t>>();
                if (leader == uut) { teams[leader] = Team(teamPlayers.size()); }
            }
        }
        std::vector<std::thread> threads;
        threads.reserve(devices.size());
        for (auto& uut : devices) {
            threads.emplace_back([&, uut, team] {
                if (FAILED(SetThreadDescription(GetCurrentThread(), std::format(L"UUT {}", uut).c_str()))) {
                    BR_LOG_ERROR("Orchestrator", "Unable to set thread name");
                }
                if (m_uutStates[uut] == UutState::Disabled) { return; }
                // states[] is not yet populated, each call will modify it
                // thus, we must have a mutex here
                mutex.lock();
                sol::state_view lua = states[uut];
                mutex.unlock();
                InitLua(lua, uut, Stage::execution);
                lua["Context"]["info"]["serial"] = serials[uut];
                // LoadIb(lua);
                LoadEnvironment(lua, m_environment);
                LoadTests(lua, m_testsDir);
                if (hasTeam) {
                    std::lock_guard lock {mutex};
                    int             leader   = team["Context"]["team"]["players"][uut]["leader"];
                    int             position = team["Context"]["team"]["players"][uut]["position"];
                    teams[leader].InitializeState(lua, uut, position, uut == leader);
                }

                sol::protected_function run = lua.script("return function(fp) Orchestrator.LoadSolution(fp) end");
                run.error_handler           = lua.script_file("lua/core/framework/error_handler.lua");
                auto result                 = run(solutionFile);
                if (!result.valid()) {
                    sol::error err = result;
                    lua["Log"]["E"](err.what());
                }
                results[uut] = result.valid();
            });
        }
        for (auto& thread : threads) {
            thread.join();
        }
        UpdateUutState(UutState::Waiting, devices);
    }
    if (hasCrashed()) { return; }

    for (std::size_t is = 1; is <= m_solution.sections.size(); ++is) {
        for (sol::object& stage : stages) {
            auto devices = stage.as<std::vector<std::size_t>>();
            UpdateUutState(UutState::Running, devices);
            std::vector<std::thread> threads;
            threads.reserve(devices.size());
            for (auto& uut : devices) {
                threads.emplace_back([&, uut] {
                    if (FAILED(SetThreadDescription(GetCurrentThread(), std::format(L"UUT {}", uut).c_str()))) {
                        BR_LOG_ERROR("Orchestrator", "Unable to set thread name");
                    }
                    if (m_uutStates[uut] == UutState::Disabled) { return; }
                    mutex.lock();
                    sol::state_view lua = states[uut];
                    mutex.unlock();
                    sol::protected_function run = lua.script("return function(is) Orchestrator.ExecuteSection(is) end");
                    run.error_handler           = lua.script_file("lua/core/framework/error_handler.lua");
                    auto result                 = run(is);
                    if (!result.valid()) {
                        sol::error err = result;
                        BR_LUA_ERROR(err.what());
                        lua["Log"]["e"](err.what());
                    }
                    results[uut] = result.valid();
                });
            }
            for (auto& thread : threads) {
                thread.join();
            }
            UpdateUutState(UutState::Waiting, devices);
        }
        if (hasCrashed()) { return; }
    }

    for (sol::object& stage : stages) {
        auto devices = stage.as<std::vector<std::size_t>>();
        UpdateUutState(UutState::Running, devices);
        std::vector<std::thread> threads;
        threads.reserve(devices.size());
        for (auto& uut : devices) {
            threads.emplace_back([&, uut] {
                if (FAILED(SetThreadDescription(GetCurrentThread(), std::format(L"UUT {}", uut).c_str()))) {
                    BR_LOG_ERROR("Orchestrator", "Unable to set thread name");
                }
                if (m_uutStates[uut] == UutState::Disabled) { return; }
                mutex.lock();
                sol::state& lua = states[uut];
                mutex.unlock();
                sol::protected_function run =
                  lua.script("return function(dir) Orchestrator.CompileExecutionResults(dir) end");
                run.error_handler = lua.script_file("lua/core/framework/error_handler.lua");
                auto result       = run(std::format("{}/{}", m_outputDirectory, lastSubdirectory));
                if (!result.valid()) {
                    sol::error err = result;
                    lua["Log"]["E"](err.what());
                }
                results[uut] = result.valid();
            });
        }
        for (auto& thread : threads) {
            thread.join();
        }
    }
    if (hasCrashed()) { return; }

    for (sol::object& stage : stages) {
        auto devices = stage.as<std::vector<std::size_t>>();
        CheckResults(devices);
    }
}

void Orchestrator::CheckResults(const std::vector<std::size_t>& devices)
{
    FRASY_PROFILE_FUNCTION();
    using nlohmann::json;
    for (auto& uut : devices) {
        std::string resultFile = std::format("{}/{}/{}.json", m_outputDirectory, lastSubdirectory, uut);
        if (std::filesystem::exists(resultFile)) {
            std::ifstream ifs {resultFile};
            std::string   content = std::string(std::istreambuf_iterator<char> {ifs}, {});
            json          data    = json::parse(content);
            bool          passed  = data["info"]["pass"];
            std::string   serial  = data["info"]["serial"];
            m_uutStates[uut]      = passed ? UutState::Passed : UutState::Failed;
            std::filesystem::copy(resultFile,
                                  std::format("{}/{}/{}_{}.txt",
                                              m_outputDirectory,
                                              passed ? passSubdirectory : failSubdirectory,
                                              std::chrono::system_clock::now().time_since_epoch().count(),
                                              serial));
        }
        else if (m_uutStates[uut] != UutState::Disabled) {
            m_uutStates[uut] = UutState::Error;
            BR_LUA_ERROR("Missing report for UUT {}, files '{}' does not exist.", uut, resultFile);
        }
    }
}

void Orchestrator::ToggleUut(std::size_t index)
{
    if (isRunning()) { return; }
    auto state   = m_uutStates[index] == UutState::Disabled ? UutState::Idle : UutState::Disabled;
    bool hasTeam = (*m_state)["Team"]["HasTeam"]();
    if (hasTeam) {
        std::size_t leader = (*m_state)["Context"]["team"]["players"][index]["leader"];
        auto        team   = (*m_state)["Context"]["team"]["teams"][leader].get<std::vector<std::size_t>>();
        UpdateUutState(state, team, true);
    }
    else {
        m_uutStates[index] = state;
    }
}

void Orchestrator::Generate()
{
    if (isRunning()) {
        Brigerad::warningDialog("Frasy", "Orchestrator is busy");
        return;
    }

    m_running = std::async(std::launch::async, [this] { RunStageGenerate(true); });
}

void Orchestrator::SetTestEnable(const std::string& sequence, const std::string& test, bool enable)
{
    m_solution.SetTestEnable(sequence, test, enable);
}

void Orchestrator::SetSequenceEnable(const std::string& sequence, bool enable)
{
    m_solution.SetSequenceEnable(sequence, enable);
}

bool Orchestrator::isRunning() const
{
    using namespace std::chrono_literals;
    return m_running.valid() && m_running.wait_for(10us) == std::future_status::timeout;
}

[[nodiscard]] UutState Orchestrator::GetUutState(std::size_t uut) const
{
    return uut < m_uutStates.size() ? m_uutStates[uut] : UutState::Idle;
}

void Orchestrator::setLoadUserFunctions(const std::function<void(sol::state_view)>& callback)
{
    m_loadUserFunctions = callback;
}

void Orchestrator::setLoadUserBoards(const std::function<sol::table(sol::state_view)>& callback)
{
    m_loadUserBoards = callback;
}

void Orchestrator::setLoadUserValues(const std::function<sol::table(sol::state_view)>& callback)
{
    m_loadUserValues = callback;
}

// </editor-fold>


// <editor-fold desc="exclusive">
void Orchestrator::ImportExclusive(sol::state_view lua, Stage stage)
{
    if (!m_exclusiveLock) { m_exclusiveLock = std::make_unique<std::mutex>(); }
    switch (stage) {
        case Stage::execution:
            lua["__exclusive"] = [&](std::size_t index, sol::function func) {
                FRASY_PROFILE_FUNCTION();
                m_exclusiveLock->lock();
                auto& mutex = m_exclusiveLockMap[index];
                m_exclusiveLock->unlock();
                std::lock_guard lock {mutex};
                std::cout << "Exclusive part: Start " << std::endl;
                func();
                std::cout << "Exclusive part: End " << std::endl;
            };
            break;

        case Stage::idle:
        case Stage::generation:
        case Stage::validation:
        default:
            lua["__exclusive"] = [&](std::size_t index, sol::function func) {
                FRASY_PROFILE_FUNCTION();
                func();
            };
            break;
    }
}
// </editor-fold>

// <editor-fold desc="init">// </editor-fold>

// <editor-fold desc="log">
void Orchestrator::ImportLog(sol::state_view lua, std::size_t uut, [[maybe_unused]] Stage stage)
{
    lua.script_file("lua/core/sdk/log.lua");
    lua["Log"]["C"] = [uut](const std::string& message) { BR_LOG_CRITICAL(std::format("UUT{}", uut), message); };
    lua["Log"]["E"] = [uut](const std::string& message) { BR_LOG_ERROR(std::format("UUT{}", uut), message); };
    lua["Log"]["W"] = [uut](const std::string& message) { BR_LOG_WARN(std::format("UUT{}", uut), message); };
    lua["Log"]["I"] = [uut](const std::string& message) { BR_LOG_INFO(std::format("UUT{}", uut), message); };
    lua["Log"]["D"] = [uut](const std::string& message) { BR_LOG_DEBUG(std::format("UUT{}", uut), message); };
    lua["Log"]["T"] = [uut](const std::string& message) { BR_LOG_TRACE(std::format("UUT{}", uut), message); };
}
// </editor-fold>

// <editor-fold desc="populate_map">
void Orchestrator::PopulateMap()
{
    m_map = {};
    for (auto& [k, v] : (*m_state)["Context"]["map"]["ibs"].get<sol::table>()) {
        auto ib = v.as<sol::table>();
        m_map.ibs.emplace_back(static_cast<int>(ib["ib"]["kind"].get<std::size_t>()),
                               static_cast<int>(ib["ib"]["nodeId"].get<std::size_t>()));
    }

    for (auto& [k, v] : (*m_state)["Context"]["map"]["uuts"].get<sol::table>()) {
        m_map.uuts.emplace_back(v.as<std::size_t>());
    }

    // TODO load teams
    //
    //    if ((*m_state)["Context"]["team"]["HasTeam"]()) {
    //        for (auto& [k, v] : (*m_state)["Context"]["team"]["teams"].get<sol::table>()) {
    //            m_map.teams.emplace_back();
    //        }
    //    }
}

// </editor-fold>

// <editor-fold desc="popup">
void Orchestrator::RenderPopups()
{
    std::lock_guard lock {(*m_popupMutex)};
    for (auto& [name, popup] : m_popups) {
        popup->Render();
    }
}

void Orchestrator::ImportPopup(sol::state_view lua, std::size_t uut, Stage stage)
{
    lua.script_file("lua/core/sdk/popup.lua");
    lua["__popup"]            = lua.create_table();
    lua["__popup"]["Consume"] = [&, uut](sol::table builder) { m_popups[Popup::GetName(uut, builder)]->Consume(); };
    if (stage == Stage::execution) {
        lua["__popup"]["Show"] = [&, uut](sol::table builder) {
            Popup popup = Popup(uut, builder);
            m_popupMutex->lock();
            m_popups[popup.GetName()] = &popup;
            m_popupMutex->unlock();
            popup.Routine();
            m_popupMutex->lock();
            m_popups.erase(popup.GetName());
            m_popupMutex->unlock();
            return popup.GetInputs();
        };
    }
    else if (stage == Stage::validation) {
        lua["__popup"]["Show"] = [&, uut](sol::table builder) {
            Popup popup = Popup(uut, builder);
            popup.Routine();
            return popup.GetInputs();
        };
    }
    else {
        lua["__popup"]["Show"] = [&, uut](sol::table builder) {
            Popup popup = Popup(uut, builder);
            return popup.GetInputs();
        };
    }
}
// </editor-fold>

// <editor-fold desc="update_uut_state">
void Orchestrator::UpdateUutState(enum UutState state, bool force)
{
    std::vector<std::size_t> uuts;
    UpdateUutState(state, m_map.uuts, force);
}

void Orchestrator::UpdateUutState(enum UutState state, const std::vector<std::size_t>& uuts, bool force)
{
    for (auto uut : uuts) {
        if (m_uutStates[uut] == UutState::Disabled && !force) { continue; }
        m_uutStates[uut] = state;
    }
}
// </editor-fold>
}    // namespace Frasy::Lua
