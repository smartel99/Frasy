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
#include "../table_deserializer.h"
#include "../table_serializer.h"
#include "../tag.h"
#include "../team.h"

#include <Brigerad/Utils/dialogs/warning.h>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <json.hpp>

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

void Orchestrator::DoTests(const std::vector<std::string>& serials, bool regenerate)
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
    m_running = std::async(std::launch::async, [this, &serials, regenerate] { RunTests(serials, regenerate); });
}

bool Orchestrator::CreateOutputDirs()
{
    namespace fs = std::filesystem;
    try
    {
        fs::create_directory(m_outputDirectory);
        fs::create_directory(std::format("{}/{}", m_outputDirectory, passDirectory));
        fs::create_directory(std::format("{}/{}", m_outputDirectory, failDirectory));

        const auto last = std::format("{}/{}", m_outputDirectory, lastDirectory);
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


void Orchestrator::InitLua(sol::state& lua, std::size_t uut, Stage stage)
{
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

    lua["Context"]["stage"]   = lua["Stage"][stage2str(stage)];
    lua["Context"]["uut"]     = uut;
    lua["Context"]["version"] = "0.1.0";
    std::atomic_thread_fence(std::memory_order_release);

    // Utils
    lua.require_file("Utils", "lua/core/utils/module.lua");
    lua["Utils"]["dirlist"] = [](const std::string& dir)
    {
        namespace fs = std::filesystem;
        std::vector<std::string> files;
        for (auto const& dir_entry : fs::directory_iterator {dir})
        {
            auto file = dir_entry.path().string();
            files.push_back(file.substr(0, file.size() - 4));
        }
        return sol::as_table(files);
    };
    lua["Utils"]["sleep_for"] = [](int duration) { std::this_thread::sleep_for(std::chrono::milliseconds(duration)); };
    lua.require_file("Json", "lua/core/vendor/json.lua");
    ImportLog(lua, uut, stage);
    ImportPopup(lua, uut, stage);
    ImportSync(lua, stage);
    ImportExclusive(lua, stage);
    lua.script_file("lua/core/framework/exception.lua");

    // Framework
    lua.script_file("lua/core/sdk/environment/team.lua");
    lua.script_file("lua/core/sdk/environment/environment.lua");
    lua.script_file("lua/core/sdk/testbench.lua");
    lua.script_file("lua/core/framework/orchestrator.lua");
    lua.script_file("lua/core/sdk/test.lua");

    m_populateUserMethods(lua, stage);
}

void Orchestrator::LoadIb(sol::state& lua)
{
    if (!m_ibEnabled) { return; }
    using Frasy::Communication::DeviceMap;
    using Frasy::Communication::SerialDevice;

    DeviceMap& devices = DeviceMap::Get();

    if (devices.IsScanning()) { throw std::runtime_error("Cannot load IB when DeviceMap is scanning"); }

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
        BR_LUA_DEBUG("Command {}", fun.Name);
        if (isExecution) { LoadIbCommandForExecution(lua, fun); }
        else { LoadIbCommandForValidation(lua, fun); }
    }
}

void Orchestrator::LoadIbCommandForValidation(sol::state& lua, const Frasy::Actions::CommandInfo::Reply& fun)
{
    using Frasy::Communication::DeviceMap;
    using Frasy::Communication::SerialDevice;
    DeviceMap& devices                                = DeviceMap::Get();
    lua["Context"]["Testbench"]["commands"][fun.Name] = [&](std::size_t        ib,
                                                            sol::variadic_args args) -> std::optional<sol::table>
    {
        SerialDevice&                    device = devices[ib - 1];
        std::vector<Type::Struct::Field> fields;
        fields.reserve(fun.Parameters.size());
        for (const auto& value : fun.Parameters) { fields.push_back({value.Name, value.Type, value.Count}); }
        CheckArgs(lua, device.GetTypeManager(), fields, args);
        return {};
    };
}

void Orchestrator::LoadIbCommandForExecution(sol::state& lua, const Frasy::Actions::CommandInfo::Reply& fun)
{
    namespace fc = Frasy::Communication;

    fc::DeviceMap& devices                            = fc::DeviceMap::Get();
    lua["Context"]["Testbench"]["commands"][fun.Name] = [&](std::size_t        ib,
                                                            sol::variadic_args args) -> std::optional<sol::table>
    {
        try
        {
            fc::SerialDevice&                    device = devices[ib - 1];
            fc::Packet                           packet;
            sol::table                       table = lua.create_table();
            std::vector<Type::Struct::Field> fields;
            fields.reserve(fun.Parameters.size());
            for (const auto& value : fun.Parameters) { fields.push_back({value.Name, value.Type, value.Count}); }
            Lua::ArgsToTable(table, device.GetTypeManager(), fields, args);
            Lua::ParseTable(table, device.GetTypeManager(), fields, packet.Payload);

            packet.Header.CommandId     = fun.Id;
            packet.Header.TransactionId = fc::AUTOMATIC_TRANSACTION_ID;
            packet.UpdatePayloadSize();
            std::size_t          tries = 10;
            std::vector<uint8_t> response;
            while (tries-- != 0)
            {
                auto resp = device.Transmit(packet).Collect();
                using Frasy::Actions::CommandId;
                if (resp.Header.CommandId == static_cast<fc::cmd_id_t>(CommandId::Status))
                {
                    auto status = resp.FromPayload<Frasy::Actions::Status::Reply>();
                    lua["Log"]["w"](std::format("Received status '{}' : {}",
                                                Frasy::Actions::Status::ErrorCode::ToStr(status.Code),
                                                status.Message));
                }
                else if (resp.Header.CommandId == fun.Id)
                {
                    response = resp.Payload;
                    break;
                }
                else { lua["Log"]["e"](std::format("Unknown command: {}", resp.Header.CommandId)); }
                {
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(50ms);
                }
            }
            table = Lua::Deserialize(lua, fun.Returns, device.GetStructs(), device.GetEnums(), response);
            return table;
        }
        catch (const std::exception& e)
        {
            return {};
        }
    };
}

bool Orchestrator::LoadEnvironment(sol::state& lua, const std::string& filename)
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

bool Orchestrator::LoadTests(sol::state& lua, const std::string& filename)
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

bool Orchestrator::DoStep(sol::state& lua, const std::string& filename)
{
    sol::protected_function run = lua.script_file(filename);
    run.error_handler           = lua.script_file("lua/core/framework/error_handler.lua");
    auto result                 = run(std::format("{}/{}", m_outputDirectory, lastDirectory));
    if (!result.valid())
    {
        sol::error err = result;
        lua["Log"]["e"](err.what());
    }
    else { lua["Log"]["i"]("Success"); }
    return result.valid();
}

void Orchestrator::RunTests(const std::vector<std::string>& serials, bool regenerate)
{
    UpdateUutState(UutState::Waiting);
    if (!CreateOutputDirs())
    {
        BR_LUA_ERROR("Failed to create logs directories");
        UpdateUutState(UutState::Idle);
        return;
    }
    if (!Generate(regenerate))
    {
        BR_LUA_ERROR("Generation failed");
        UpdateUutState(UutState::Error);
        return;
    }
    if (!Verify(*m_state))
    {
        BR_LUA_ERROR("Verification failed");
        UpdateUutState(UutState::Error);
        return;
    }
    Execute(*m_state, serials);
}

bool Orchestrator::Generate(bool regenerate)
{
    if (m_generated && !regenerate) { return true; }

    sol::state lua;
    InitLua(lua, 1);
    LoadIb(lua);
    LoadEnvironment(lua, m_environment);
    LoadTests(lua, m_testsDir);
    m_generated = DoStep(lua, "lua/core/helper/generate.lua");
    return m_generated;
}

bool Orchestrator::Verify(const sol::state& team)
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
                if (leader == uut) teams[leader] = Team(teamPlayers.size());
            }
        }
        std::vector<std::thread>    threads;
        std::mutex                  mutex;
        std::map<std::size_t, bool> results;
        for (auto& uut : devices)
        {
            if (m_uutStates[uut] == UutState::Disabled) { continue; }
            threads.emplace_back(
              [&, uut]
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
                  bool            result = DoStep(lua, "lua/core/helper/validate.lua");
                  std::lock_guard lock {mutex};
                  results[uut] = result;
              });
        }
        for (auto& thread : threads) { thread.join(); }
        size_t expectedResults = std::accumulate(devices.begin(),
                                                 devices.end(),
                                                 size_t(0),
                                                 [&](size_t tot, const auto& uut)
                                                 { return tot + (m_uutStates[uut] == UutState::Disabled ? 0 : 1); });
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

void Orchestrator::Execute(const sol::state& team, const std::vector<std::string>& serials)
{
    bool hasTeam = team["Team"]["HasTeam"]();
    auto stages  = team["Context"]["Worker"]["stages"].get<std::vector<sol::object>>();
    for (sol::object& stage : stages)
    {
        std::map<std::size_t, Team> teams;
        auto                        devices = stage.as<std::vector<std::size_t>>();
        UpdateUutState(UutState::Running, devices);
        m_globalSync = std::make_unique<std::barrier<>>(devices.size());
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
        std::mutex                  mutex;
        std::map<std::size_t, bool> results;
        for (auto& uut : devices)
        {
            threads.emplace_back(
              [&, uut]
              {
                  if (m_uutStates[uut] == UutState::Disabled) { return; }
                  sol::state lua;
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
                  bool            result = DoStep(lua, "lua/core/helper/execute.lua");
                  std::lock_guard lock {mutex};
                  results[uut] = result;
              });
        }
        for (auto& thread : threads) { thread.join(); }
        CheckResults(devices);
    }
}

void Orchestrator::CheckResults(const std::vector<std::size_t>& devices)
{
    using nlohmann::json;
    for (auto& uut : devices)
    {
        std::string resultFile = std::format("{}/{}/{}.json", m_outputDirectory, lastDirectory, uut);
        if (std::filesystem::exists(resultFile))
        {
            std::ifstream ifs {resultFile};
            std::string   content = std::string(std::istreambuf_iterator<char> {ifs}, {});
            json          data    = json::parse(content);
            bool          passed  = data["pass"];
            std::string   serial  = data["serial"];
            m_uutStates[uut]      = passed ? UutState::Passed : UutState::Failed;
            std::filesystem::copy(resultFile,
                                  std::format("{}/{}/{}_{}.txt",
                                              m_outputDirectory,
                                              passed ? passDirectory : failDirectory,
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

bool Orchestrator::IsRunning() const
{
    using namespace std::chrono_literals;
    return m_running.valid() && m_running.wait_for(10us) == std::future_status::timeout;
}

[[nodiscard]] UutState Orchestrator::UutState(std::size_t uut) const
{
    return uut < m_uutStates.size() ? m_uutStates[uut] : UutState::Idle;
}
// </editor-fold>

// <editor-fold desc="exclusive">
void Orchestrator::ImportExclusive(sol::state& lua, Stage stage)
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

// <editor-fold desc="init">
bool Orchestrator::Init(const std::string& environment, const std::string& testsDir)
{
    m_state       = std::make_unique<sol::state>();
    m_map         = {};
    m_generated   = false;
    m_environment = environment;
    m_testsDir    = testsDir;
    InitLua(*m_state);
    if (!LoadEnvironment(*m_state, m_environment)) { return false; }
    if (!LoadTests(*m_state, m_testsDir)) { return false; }
    PopulateMap();
    m_uutStates.resize(m_map.count.uut + 1, UutState::Idle);
    m_popupMutex = std::make_unique<std::mutex>();

    return true;
}
// </editor-fold>

// <editor-fold desc="log">
void Orchestrator::ImportLog(sol::state& lua, std::size_t uut, [[maybe_unused]] Stage stage)
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

void Orchestrator::ImportPopup(sol::state& lua, std::size_t uut, Stage stage)
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

// <editor-fold desc="sync">
void Orchestrator::ImportSync(sol::state& lua, Stage stage)
{
    lua.script_file("lua/core/sdk/sync.lua");
    if (stage != Stage::Execution) return;
    lua["Sync"]["Global"] = [&]() { m_globalSync->arrive_and_wait(); };
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
