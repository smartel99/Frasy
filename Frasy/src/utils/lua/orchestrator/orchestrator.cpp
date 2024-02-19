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
#include "../../communication/serial/device_map.h"
#include "../args_checker.h"
#include "../dummy_table_deserializer.h"
#include "../table_deserializer.h"
#include "../table_serializer.h"
#include "../tag.h"
#include "../team.h"
#include "utils/commands/type/manager/manager.h"
#include "utils/lua/save_as_json.h"
#include "utils/misc/serializer.h"

#include <Brigerad/Utils/dialogs/warning.h>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <json.hpp>
#include <stdexcept>

namespace Frasy::Lua
{

// <editor-fold desc="Orchestrator">
std::string Orchestrator::stage2str(Frasy::Lua::Orchestrator::Stage stage)
{
    switch (stage)
    {
        case Stage::Generation: return "Generation";
        case Stage::Validation: return "Validation";
        case Stage::Execution: return "Execution";
        case Stage::Idle:
        default: return "Idle";
    }
}

// <editor-fold desc="Test related">
namespace
{
int OnPanic(lua_State* lua)
{    // Get the error message from the top of the stack.
    std::string msg;
    if (lua_isstring(lua, -1)) { msg = lua_tostring(lua, -1); }
    else { msg = "[No message provided]"; }
    BR_LUA_CRITICAL("An error occurred:");
    BR_LUA_ERROR("{}\n\r", msg);
    luaL_traceback(lua, lua, nullptr, 0);
    msg = lua_tostring(lua, -1);
    BR_LUA_ERROR("{}\n\r", msg);
    //    luaL_dostring(lua, "Log.Error(debug.traceback())");

    return 0;
}
}    // namespace


bool Orchestrator::LoadUserFiles(const std::string& environment, const std::string& testsDir)
{
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
    m_uutStates.resize(m_map.count.uut + 1, UutState::Idle);

    return true;
}
// </editor-fold>


void Orchestrator::RunSolution(const std::vector<std::string>& serials, bool regenerate, bool skipVerification)
{
    if (IsRunning())
    {
        Brigerad::WarningDialog("Frasy", "Test is already running!");
        return;
    }
    if (m_map.count.uut == 0)
    {
        Brigerad::WarningDialog("Frasy", "No UUTs to test!");
        return;
    }
    m_running = std::async(
      std::launch::async,
      [this, &serials, regenerate, skipVerification] { RunTests(serials, regenerate, skipVerification); });
}

const Models::Solution& Orchestrator::GetSolution()
{
    return m_solution;
}

bool Orchestrator::CreateOutputDirs()
{
    namespace fs = std::filesystem;
    try
    {
        fs::create_directory(m_outputDirectory);
        fs::create_directory(std::format("{}/{}", m_outputDirectory, passSubdirectory));
        fs::create_directory(std::format("{}/{}", m_outputDirectory, failSubdirectory));

        const auto last = std::format("{}/{}", m_outputDirectory, lastSubdirectory);
        fs::remove_all(last);
        fs::create_directory(last);
    }
    catch (const fs::filesystem_error& e)
    {
        BR_LUA_ERROR(e.what());
        return false;
    }
    return true;
}

bool Orchestrator::InitLua(sol::state_view lua, std::size_t uut, Stage stage)
{
    try
    {
        lua.open_libraries(
          sol::lib::debug,
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

        lua["Context"]["stage"]   = lua["Stage"][stage2str(stage)];
        lua["Context"]["uut"]     = uut;
        lua["Context"]["version"] = "0.1.0";

        // TODO Add information from instrumentation card to context.
        auto& devices = Communication::DeviceMap::Get();
        if (!devices.IsScanning())
        {
            // We would block the thread if the devices are currently being scanned.
            lua["Context"]["ibs"] = lua.create_table(0, devices.size());
            for (auto&& [id, info] : devices)
            {
                const auto& deviceInfo = info.GetInfo();

                lua["Context"]["ibs"][id]         = lua.create_table(0, 5);
                lua["Context"]["ibs"][id]["Uuid"] = deviceInfo.Uuid;
                lua["Context"]["ibs"][id]["Id"]   = deviceInfo.Id;

                auto trim = [](std::string_view s)
                {
                    size_t pos = s.find_last_not_of('\u0000');
                    return s.substr(0, pos + 1);
                };

                lua["Context"]["ibs"][id]["Version"] = trim(deviceInfo.Version);
                lua["Context"]["ibs"][id]["PrjName"] = trim(deviceInfo.PrjName);
                lua["Context"]["ibs"][id]["Built"]   = trim(deviceInfo.Built);
            }
        }

        std::atomic_thread_fence(std::memory_order_release);

        // Utils
        lua.require_file("Utils", "lua/core/utils/module.lua");
        lua["Utils"]["dirlist"] = [](const std::string& dir)
        {
            namespace fs = std::filesystem;
            std::vector<std::string> files;
            for (auto const& dir_entry : fs::directory_iterator {dir})
            {
                if (dir_entry.is_regular_file())
                {
                    // Just add lua files, not directories or other random files.
                    auto file = dir_entry.path();
                    if (file.extension() == ".lua")
                    {
                        file.replace_extension();    // Remove the extension for files
                        files.push_back(file.string());
                    }
                }
            }
            return sol::as_table(files);
        };
        lua["Utils"]["sleep_for"]    = stage == Stage::Execution ? [](int duration)
        { std::this_thread::sleep_for(std::chrono::milliseconds(duration)); }
                                                                 : [](int duration) {};
        lua["Utils"]["save_as_json"] = [](sol::table table, const std::string& file) { SaveAsJson(table, file); };
        lua.require_file("Json", "lua/core/vendor/json.lua");
        ImportLog(lua, uut, stage);
        ImportPopup(lua, uut, stage);
        ImportExclusive(lua, stage);
        lua.script_file("lua/core/framework/exception.lua");

        // Framework
        lua.script_file("lua/core/sdk/environment/team.lua");
        lua.script_file("lua/core/sdk/environment/environment.lua");
        lua.script_file("lua/core/sdk/testbench.lua");
        lua.script_file("lua/core/framework/orchestrator.lua");
        lua.script_file("lua/core/sdk/test.lua");

        m_populateUserMethods(lua, stage);

        // Debug functionalities.
        lua.do_file("lua/core/helper/debug.lua");

        return true;
    }
    catch (std::exception& e)
    {
        BR_LUA_ERROR("An error occurred while loading the framework: {}", e.what());
        return false;
    }
}

// <editor-fold desc="Loading and executing">
void Orchestrator::LoadIb(sol::state_view lua)
{
    if (!m_ibEnabled) { return; }
    using Frasy::Communication::DeviceMap;
    using Frasy::Communication::SerialDevice;

    DeviceMap& devices = DeviceMap::Get();

    if (devices.empty()) { throw std::runtime_error("No devices connected!"); }
    else if (devices.IsScanning()) { throw std::runtime_error("Cannot load IB when DeviceMap is scanning"); }

    SerialDevice& device = devices.begin()->second;
    for (const auto& [_, e] : device.GetEnums())
    {
        lua.create_named_table(e.Name);
        for (const auto& field : e.Fields) { lua[e.Name][field.Name] = field.Value; }
    }

    bool isExecution = lua["Context"]["stage"].get<int>() == lua["Stage"]["Execution"].get<int>();

    BR_LUA_DEBUG("Loading commands");
    for (const auto& [id, fun] : device.GetCommands())
    {
        if (isExecution) { LoadIbCommandForExecution(lua, fun); }
        else { LoadIbCommandForValidation(lua, fun); }
    }
}

void Orchestrator::LoadIbCommandForValidation(sol::state_view lua, const Frasy::Actions::CommandInfo::Reply& fun)
{
    using Frasy::Communication::DeviceMap;
    using Frasy::Communication::SerialDevice;
    DeviceMap& devices = DeviceMap::Get();
    lua["Context"]["Testbench"]["commands"][fun.Name] =
      [&, lua](std::size_t ib, sol::variadic_args args) -> std::optional<sol::table>
    {
        SerialDevice&                    device = GetDevForIb(ib);
        std::vector<Type::Struct::Field> fields;
        fields.reserve(fun.Parameters.size());
        for (const auto& value : fun.Parameters) { fields.push_back({value.Name, value.Type, value.Count}); }
        CheckArgs(lua, device.GetTypeManager(), fields, args);

        Lua::DummyDeserializer deserializer {lua, fun.Returns, device.GetStructs(), device.GetEnums()};
        return deserializer.Deserialize();
    };
}

void Orchestrator::LoadIbCommandForExecution(sol::state_view lua, const Frasy::Actions::CommandInfo::Reply& fun)
{
    using Frasy::Communication::DeviceMap;
    using Frasy::Communication::SerialDevice;
    DeviceMap& devices = DeviceMap::Get();
    lua["Context"]["Testbench"]["commands"][fun.Name] =
      [&, lua](std::size_t ib, sol::variadic_args args) mutable -> std::optional<sol::table>
    {
        try
        {
            SerialDevice&                    device = GetDevForIb(ib);
            Communication::Packet            packet;
            sol::table                       table = lua.create_table();
            std::vector<Type::Struct::Field> fields;
            fields.reserve(fun.Parameters.size());
            for (const auto& value : fun.Parameters) { fields.push_back({value.Name, value.Type, value.Count}); }
            Lua::ArgsToTable(table, device.GetTypeManager(), fields, args);
            Lua::ParseTable(table, device.GetTypeManager(), fields, packet.Payload);

            packet.Header.CommandId     = fun.Id;
            packet.Header.TransactionId = Communication::AUTOMATIC_TRANSACTION_ID;
            packet.UpdatePayloadSize();
            std::size_t          tries = 10;
            std::vector<uint8_t> response;
            while (tries-- != 0)
            {
                auto resp = device.Transmit(packet).Collect();
                using Frasy::Actions::CommandId;
                if (resp.Header.CommandId == static_cast<Actions::cmd_id_t>(CommandId::Status))
                {
                    auto status = resp.FromPayload<Frasy::Actions::Status::Reply>();
                    lua["Log"]["w"](std::format(
                      "Received status '{}' : {}",
                      Frasy::Actions::Status::ErrorCode::ToStr(status.Code),
                      status.Message));
                }
                else if (resp.Header.CommandId == fun.Id)
                {
                    response = resp.Payload;
                    break;
                }
                else { lua["Log"]["e"](std::format("Unknown command: {}", resp.Header.CommandId)); }

                using namespace std::chrono_literals;
                std::this_thread::sleep_for(50ms);
            }
            Lua::Deserializer deserializer {lua, fun.Returns, device.GetStructs(), device.GetEnums()};
            auto              b = response.begin();
            auto              e = response.end();
            return deserializer.Deserialize(b, e);
        }
        catch (const std::exception& e)
        {
            lua["Log"]["e"](std::format("An error occurred while running '{}': {}", fun.Name, e.what()));
            return {};
        }
    };
}

bool Orchestrator::LoadEnvironment(sol::state_view lua, const std::string& filename)
{
    sol::protected_function run = lua.script_file("lua/core/helper/load_environment.lua");
    run.error_handler           = lua.script_file("lua/core/framework/error_handler.lua");
    auto result                 = run(filename);
    if (!result.valid())
    {
        sol::error err = result;
        lua["Log"]["e"](err.what());
    }
    else { lua["Log"]["i"]("Environment loaded successfully"); }
    return result.valid();
}

bool Orchestrator::LoadTests(sol::state_view lua, const std::string& filename)
{
    sol::protected_function run = lua.script_file("lua/core/helper/load_tests.lua");
    run.error_handler           = lua.script_file("lua/core/framework/error_handler.lua");
    auto result                 = run(filename);
    if (!result.valid())
    {
        sol::error err = result;
        lua["Log"]["e"](err.what());
    }
    else { lua["Log"]["i"]("Tests loaded successfully"); }
    return result.valid();
}

void Orchestrator::RunTests(const std::vector<std::string>& serials, bool regenerate, bool skipVerification)
{
    UpdateUutState(UutState::Waiting);
    if (!CreateOutputDirs())
    {
        BR_LUA_ERROR("Failed to create logs directories");
        UpdateUutState(UutState::Idle);
        return;
    }
    if (!RunStageGenerate(regenerate))
    {
        BR_LUA_ERROR("Generation failed");
        UpdateUutState(UutState::Error);
        return;
    }
    if (!skipVerification && !RunStageVerify(*m_state))
    {
        BR_LUA_ERROR("Verification failed");
        UpdateUutState(UutState::Error);
        return;
    }
    RunStageExecute(*m_state, serials);
}

bool Orchestrator::RunStageGenerate(bool regenerate)
{
    if (m_generated && !regenerate) { return true; }

    sol::state lua;
    InitLua(lua, 1);
    LoadIb(lua);
    LoadEnvironment(lua, m_environment);
    LoadTests(lua, m_testsDir);
    sol::protected_function run = lua.script_file("lua/core/helper/generate.lua");
    run.error_handler           = lua.script_file("lua/core/framework/error_handler.lua");
    auto result                 = run(solutionFile);
    if (!result.valid())
    {
        sol::error err = result;
        lua["Log"]["e"](err.what());
    }
    else { lua["Log"]["i"]("Success"); }
    if (result.valid())
    {
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
        for (const auto& gSc : generation)
        {
            // Section
            m_solution.sections.emplace_back();
            auto& sc = m_solution.sections.back();
            sc.reserve(gSc.size());
            for (const auto& gScS : gSc)
            {
                // Section stage
                sc.emplace_back();
                auto& scs = sc.back();
                scs.reserve(gScS.size());
                for (const auto& gSq : gScS)
                {
                    // Sequence
                    scs.emplace_back();
                    auto& sq = scs.back();
                    sq.first = gSq["name"];
                    sq.second.reserve(gSq["tests"].size());
                    if (!m_solution.sequences.contains(sq.first)) { m_solution.sequences[sq.first] = {}; }
                    for (const auto& gSqS : gSq["tests"])
                    {
                        // Sequence stages
                        sq.second.emplace_back();
                        auto& sqs = sq.second.back();
                        sqs.reserve(gSqS.size());
                        for (const auto& gT : gSqS)
                        {
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
    bool hasTeam = team["Team"]["HasTeam"]();
    auto stages  = team["Context"]["Worker"]["stages"].get<std::vector<sol::object>>();

    for (sol::object& stage : stages)
    {
        std::map<std::size_t, Team> teams;
        auto                        devices = stage.as<std::vector<std::size_t>>();
        if (hasTeam)
        {
            for (auto& uut : devices)
            {
                std::size_t leader      = team["Context"]["Team"]["players"][uut]["leader"];
                auto        teamPlayers = team["Context"]["Team"]["teams"][leader].get<std::vector<std::size_t>>();
                if (leader == uut) { teams[leader] = Team(teamPlayers.size()); }
            }
        }
        std::vector<std::thread>    threads;
        std::mutex                  mutex;
        std::map<std::size_t, bool> results;
        for (auto& uut : devices)
        {
            if (m_uutStates[uut] == UutState::Disabled) { continue; }
            threads.emplace_back(
              [&, uut, team]
              {
                  sol::state lua;
                  InitLua(lua, uut, Stage::Validation);
                  LoadIb(lua);
                  LoadEnvironment(lua, m_environment);
                  LoadTests(lua, m_testsDir);
                  if (hasTeam)
                  {
                      std::lock_guard lock {mutex};
                      int             leader   = team["Context"]["Team"]["players"][uut]["leader"];
                      int             position = team["Context"]["Team"]["players"][uut]["position"];
                      teams[leader].InitializeState(lua, uut, position, uut == leader);
                  }
                  sol::protected_function load_solution =
                    lua.script("return function(fp) Orchestrator.LoadSolution(fp) end");
                  load_solution.error_handler = lua.script_file("lua/core/framework/error_handler.lua");
                  auto rls                    = load_solution(solutionFile);
                  if (!rls.valid())
                  {
                      sol::error err = rls;
                      lua["Log"]["e"](err.what());
                      std::lock_guard lock {mutex};
                      results[uut] = false;
                      return;
                  }

                  sol::protected_function verify = lua.script("return function() Orchestrator.Validate() end");
                  verify.error_handler           = lua.script_file("lua/core/framework/error_handler.lua");
                  auto rv                        = verify();
                  if (!rv.valid())
                  {
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
        for (auto& thread : threads) { thread.join(); }
        size_t expectedResults = std::accumulate(
          devices.begin(),
          devices.end(),
          size_t(0),
          [&](size_t tot, const auto& uut) { return tot + (m_uutStates[uut] == UutState::Disabled ? 0 : 1); });
        if (results.size() != expectedResults)
        {
            BR_LUA_ERROR("Missing results from validation");
            return false;
        }
        if (std::any_of(results.begin(), results.end(), [](const auto entry) { return !entry.second; }))
        {
            BR_LUA_ERROR("Not all UUT passed validation");
            return false;
        }
    }
    return true;
}

void Orchestrator::RunStageExecute(sol::state_view team, const std::vector<std::string>& serials)
{
    bool hasTeam = team["Team"]["HasTeam"]();
    /**
     * A stage is a team (or a group of teams).
     * This contains a list of all the stages, as per defined by the user's environment file.
     */
    auto        stages   = team["Context"]["Worker"]["stages"].get<std::vector<sol::object>>();
    std::size_t uutCount = team["Context"]["Map"]["count"]["uut"].get<std::size_t>();

    std::map<std::size_t, Team>       teams;
    std::map<std::size_t, sol::state> states;
    std::mutex                        mutex;
    std::map<std::size_t, bool>       results;

    for (std::size_t uut = 0; uut <= uutCount; ++uut) { states[uut] = sol::state(); }

    auto hasCrashed = [&]()
    {
        if (std::any_of(results.begin(), results.end(), [](const auto& kvp) { return !kvp.second; }))
        {
            for (std::size_t uut = 1; uut <= uutCount; ++uut)
            {
                UpdateUutState(results[uut] ? UutState::Idle : UutState::Error, {uut}, false);
            }
            return true;
        }
        return false;
    };

    // Initialize lua states and teams
    for (sol::object& stage : stages)
    {
        auto devices = stage.as<std::vector<std::size_t>>();
        UpdateUutState(UutState::Running, devices);
        if (hasTeam)
        {
            for (auto& uut : devices)
            {
                std::size_t leader      = team["Context"]["Team"]["players"][uut]["leader"];
                auto        teamPlayers = team["Context"]["Team"]["teams"][leader].get<std::vector<std::size_t>>();
                if (leader == uut) { teams[leader] = Team(teamPlayers.size()); }
            }
        }
        std::vector<std::thread> threads;
        threads.reserve(devices.size());
        for (auto& uut : devices)
        {
            threads.emplace_back(
              [&, uut, team]
              {
                  if (m_uutStates[uut] == UutState::Disabled) { return; }
                  // states[] is not yet populated, each call will modify it
                  // thus, we must have a mutex here
                  mutex.lock();
                  sol::state_view lua = states[uut];
                  mutex.unlock();
                  InitLua(lua, uut, Stage::Execution);
                  lua["Context"]["serial"] = serials[uut];
                  LoadIb(lua);
                  LoadEnvironment(lua, m_environment);
                  LoadTests(lua, m_testsDir);
                  if (hasTeam)
                  {
                      std::lock_guard lock {mutex};
                      int             leader   = team["Context"]["Team"]["players"][uut]["leader"];
                      int             position = team["Context"]["Team"]["players"][uut]["position"];
                      teams[leader].InitializeState(lua, uut, position, uut == leader);
                  }

                  sol::protected_function run = lua.script("return function(fp) Orchestrator.LoadSolution(fp) end");
                  run.error_handler           = lua.script_file("lua/core/framework/error_handler.lua");
                  auto result                 = run(solutionFile);
                  if (!result.valid())
                  {
                      sol::error err = result;
                      lua["Log"]["e"](err.what());
                  }
                  results[uut] = result.valid();
              });
        }
        for (auto& thread : threads) { thread.join(); }
        UpdateUutState(UutState::Waiting, devices);
    }
    if (hasCrashed()) { return; }

    // Execute the tests.
    for (std::size_t is = 1; is <= m_solution.sections.size(); ++is)
    {
        for (sol::object& stage : stages)
        {
            auto devices = stage.as<std::vector<std::size_t>>();
            UpdateUutState(UutState::Running, devices);
            std::vector<std::thread> threads;
            threads.reserve(devices.size());
            for (auto& uut : devices)
            {
                threads.emplace_back(
                  [&, uut]
                  {
                      if (m_uutStates[uut] == UutState::Disabled) { return; }
                      mutex.lock();
                      sol::state_view lua = states[uut];
                      mutex.unlock();
                      sol::protected_function run =
                        lua.script("return function(is) Orchestrator.ExecuteSection(is) end");
                      run.error_handler = lua.script_file("lua/core/framework/error_handler.lua");
                      auto result       = run(is);
                      if (!result.valid())
                      {
                          sol::error err = result;
                          BR_LUA_ERROR(err.what());
                          lua["Log"]["e"](err.what());
                      }
                      results[uut] = result.valid();
                  });
            }
            for (auto& thread : threads) { thread.join(); }
            UpdateUutState(UutState::Waiting, devices);
        }
        if (hasCrashed()) { return; }
    }

    for (sol::object& stage : stages)
    {
        auto devices = stage.as<std::vector<std::size_t>>();
        UpdateUutState(UutState::Running, devices);
        std::vector<std::thread> threads;
        threads.reserve(devices.size());
        for (auto& uut : devices)
        {
            threads.emplace_back(
              [&, uut]
              {
                  if (m_uutStates[uut] == UutState::Disabled) { return; }
                  mutex.lock();
                  sol::state& lua = states[uut];
                  mutex.unlock();
                  sol::protected_function run =
                    lua.script("return function(dir) Orchestrator.CompileExecutionResults(dir) end");
                  run.error_handler = lua.script_file("lua/core/framework/error_handler.lua");
                  auto result       = run(std::format("{}/{}", m_outputDirectory, lastSubdirectory));
                  if (!result.valid())
                  {
                      sol::error err = result;
                      lua["Log"]["e"](err.what());
                  }
                  results[uut] = result.valid();
              });
        }
        for (auto& thread : threads) { thread.join(); }
    }
    if (hasCrashed()) { return; }

    for (sol::object& stage : stages)
    {
        auto devices = stage.as<std::vector<std::size_t>>();
        CheckResults(devices);
    }
}

void Orchestrator::CheckResults(const std::vector<std::size_t>& devices)
{
    using nlohmann::json;
    for (auto& uut : devices)
    {
        std::string resultFile = std::format("{}/{}/{}.json", m_outputDirectory, lastSubdirectory, uut);
        if (std::filesystem::exists(resultFile))
        {
            std::ifstream ifs {resultFile};
            std::string   content = std::string(std::istreambuf_iterator<char> {ifs}, {});
            json          data    = json::parse(content);
            bool          passed  = data["info"]["pass"];
            std::string   serial  = data["info"]["serial"];
            m_uutStates[uut]      = passed ? UutState::Passed : UutState::Failed;
            std::filesystem::copy(
              resultFile,
              std::format(
                "{}/{}/{}_{}.txt",
                m_outputDirectory,
                passed ? passSubdirectory : failSubdirectory,
                std::chrono::system_clock::now().time_since_epoch().count(),
                serial));
        }
        else if (m_uutStates[uut] != UutState::Disabled)
        {
            m_uutStates[uut] = UutState::Error;
            BR_LUA_ERROR("Missing report for UUT {}, files '{}' does not exist.", uut, resultFile);
        }
    }
}

void Orchestrator::ToggleUut(std::size_t index)
{
    if (IsRunning()) { return; }
    auto state   = m_uutStates[index] == UutState::Disabled ? UutState::Idle : UutState::Disabled;
    bool hasTeam = (*m_state)["Team"]["HasTeam"]();
    if (hasTeam)
    {
        std::size_t leader = (*m_state)["Context"]["Team"]["players"][index]["leader"];
        auto        team   = (*m_state)["Context"]["Team"]["teams"][leader].get<std::vector<std::size_t>>();
        UpdateUutState(state, team, true);
    }
    else { m_uutStates[index] = state; }
}

void Orchestrator::Generate()
{
    if (IsRunning())
    {
        Brigerad::WarningDialog("Frasy", "Orchestrator is busy");
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

bool Orchestrator::IsRunning() const
{
    using namespace std::chrono_literals;
    return m_running.valid() && m_running.wait_for(10us) == std::future_status::timeout;
}

[[nodiscard]] UutState Orchestrator::GetUutState(std::size_t uut) const
{
    return uut < m_uutStates.size() ? m_uutStates[uut] : UutState::Idle;
}

void Orchestrator::SetPopulateUserMethodsCallback(std::function<void(sol::state_view, Stage)> callback)
{
    m_populateUserMethods = callback;
}
// </editor-fold>


// <editor-fold desc="exclusive">
void Orchestrator::ImportExclusive(sol::state_view lua, Stage stage)
{
    if (!m_exclusiveLock) { m_exclusiveLock = std::make_unique<std::mutex>(); }
    switch (stage)
    {
        case Stage::Execution:
            lua["__exclusive"] = [&](std::size_t index, sol::function func)
            {
                m_exclusiveLock->lock();
                auto& mutex = m_exclusiveLockMap[index];
                m_exclusiveLock->unlock();
                std::lock_guard lock {mutex};
                std::cout << "Exclusive part: Start " << std::endl;
                func();
                std::cout << "Exclusive part: End " << std::endl;
            };
            break;

        case Stage::Idle:
        case Stage::Generation:
        case Stage::Validation:
        default: lua["__exclusive"] = [&](std::size_t index, sol::function func) { func(); }; break;
    }
}
// </editor-fold>

// <editor-fold desc="init">// </editor-fold>

// <editor-fold desc="log">
void Orchestrator::ImportLog(sol::state_view lua, std::size_t uut, [[maybe_unused]] Stage stage)
{
    lua.script_file("lua/core/sdk/log.lua");
    lua["Log"]["c"] = [uut](std::string message) { BR_LOG_CRITICAL(std::format("UUT{}", uut), message); };
    lua["Log"]["e"] = [uut](std::string message) { BR_LOG_ERROR(std::format("UUT{}", uut), message); };
    lua["Log"]["w"] = [uut](std::string message) { BR_LOG_WARN(std::format("UUT{}", uut), message); };
    lua["Log"]["i"] = [uut](std::string message) { BR_LOG_INFO(std::format("UUT{}", uut), message); };
    lua["Log"]["d"] = [uut](std::string message) { BR_LOG_DEBUG(std::format("UUT{}", uut), message); };
    lua["Log"]["y"] = [uut](std::string message) { BR_LOG_TRACE(std::format("UUT{}", uut), message); };
}
// </editor-fold>

// <editor-fold desc="populate_map">
void Orchestrator::PopulateMap()
{
    m_map = {};

    m_map.count.uut   = (*m_state)["Context"]["Map"]["count"]["uut"].get<std::size_t>();
    m_map.count.ib    = (*m_state)["Context"]["Map"]["count"]["ib"].get<std::size_t>();
    m_map.count.teams = (*m_state)["Context"]["Team"]["teams"].get<std::vector<sol::object>>().size();

    if (m_map.count.teams != 0)
    {
        for (auto& [k, v] : (*m_state)["Context"]["Team"]["teams"].get<sol::table>())
        {
            std::size_t leader               = k.as<std::size_t>();
            std::size_t ib                   = (*m_state)["Context"]["Map"]["uut"][leader]["ib"].get<std::size_t>();
            m_map.ibs[ib].teams[leader].uuts = v.as<std::vector<std::size_t>>();
        }
    }
    else
    {
        for (auto& [ib, ibt] : (*m_state)["Context"]["Map"]["ib"].get<sol::table>())
        {
            for (auto& [_, uut] : ibt.as<sol::table>()["uut"].get<sol::table>())
            {
                m_map.ibs[ib.as<std::size_t>()].teams[uut.as<std::size_t>()].uuts = {uut.as<std::size_t>()};
            }
        }
    }

    for (std::size_t i = 1; i <= m_map.count.uut; ++i) { m_map.uuts.push_back(i); }
}
// </editor-fold>

// <editor-fold desc="popup">
void Orchestrator::RenderPopups()
{
    std::lock_guard lock {(*m_popupMutex)};
    for (auto& [name, popup] : m_popups) { popup->Render(); }
}

void Orchestrator::ImportPopup(sol::state_view lua, std::size_t uut, Stage stage)
{
    lua.script_file("lua/core/sdk/popup.lua");
    lua["__popup"]            = lua.create_table();
    lua["__popup"]["consume"] = [&, uut](sol::table builder) { m_popups[Popup::GetName(uut, builder)]->Consume(); };
    if (stage == Stage::Execution)
    {
        lua["__popup"]["show"] = [&, uut](sol::table builder)
        {
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
    else if (stage == Stage::Validation)
    {
        lua["__popup"]["show"] = [&, uut](sol::table builder)
        {
            Popup popup = Popup(uut, builder);
            popup.Routine();
            return popup.GetInputs();
        };
    }
    else
    {
        lua["__popup"]["show"] = [&, uut](sol::table builder)
        {
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
    for (auto uut : uuts)
    {
        if (m_uutStates[uut] == UutState::Disabled && !force) continue;
        m_uutStates[uut] = state;
    }
}
// </editor-fold>
}    // namespace Frasy::Lua
