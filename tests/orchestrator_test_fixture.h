/**
 * @file    orchestrator_test_fixture.h
 * @brief   Derived test fixture for orchestrator and framework testing.
 *
 * Extends LuaTestFixture with orchestrator-specific mocks (Team, profiling),
 * loads the full orchestrator module and SDK, and provides helpers for
 * scope setup and state reset between tests.
 */

#ifndef FRASY_TESTS_ORCHESTRATOR_TEST_FIXTURE_H
#define FRASY_TESTS_ORCHESTRATOR_TEST_FIXTURE_H

#include "lua_test_fixture.h"

#include <string>
#include <vector>

/**
 * Orchestrator test fixture that provides:
 * - Full Team mock with tracked calls
 * - Profiling event mocks (__profileStartEvent, __profileEndEvent)
 * - Loaded orchestrator module (Orchestrator global with all functions)
 * - Loaded SDK (Sequence, Test, Requires, Expect, Sync, Once, Exclusive)
 * - Context.info.stage set to Stage.generation by default
 * - Helper to reset orchestrator state between tests
 * - Helper to change stage
 */
class OrchestratorTestFixture : public LuaTestFixture
{
protected:
    // Track Team mock calls
    int              teamFailCount = 0;
    int              teamSyncCount = 0;
    std::vector<int> teamSyncStatuses;
    int              teamSyncReturnStatus = 0; // What __sync returns (Team.Status.pass by default)

    void SetUp() override
    {
        // Call base SetUp (opens libs, registers base mocks, loads stage/exception/global)
        LuaTestFixture::SetUp();

        // Register orchestrator-specific mocks
        registerOrchestratorMocks();

        // Set stage to generation (where sequences and tests are defined)
        lua.script("Context.info.stage = Stage.generation");

        // Remove the minimal Orchestrator table from base fixture
        // (the real one will be created by orchestrator.lua)
        lua.script("Orchestrator = nil");

        // Load the orchestrator module — this creates the real Orchestrator global
        lua.script_file(getLuaBaseDir() + "/lua/core/framework/orchestrator.lua");

        // Set up enable_list (needed for execution)
        lua.script("Context.orchestrator.enable_list = {}");

        // Load the SDK (Sequence, Test, Requires, Expect, Sync, etc.)
        lua.script_file(getLuaBaseDir() + "/lua/core/sdk/test.lua");
    }

    /**
     * Register orchestrator-specific mocks.
     * Called after base registerMocks().
     */
    void registerOrchestratorMocks()
    {
        // Team mock — tracks calls and allows configurable __sync return
        lua.script(R"(
            Team = {
                Status = { pass = 0, fail = 1, critical_failure = 2 },
                HasTeam = function() return false end,
                IsLeader = function() return true end,
            }
        )");

        lua.set_function("Team.Fail", [this]() { teamFailCount++; });

        // Team.Sync needs to be a Lua function that accesses result
        // We'll override it as a no-op that does nothing when HasTeam is false
        lua.script(R"(
            function Team.Sync(result)
                if not Team.HasTeam() then return end
            end
        )");

        // Profiling mocks
        lua.set_function("__profileStartEvent", [](const std::string&, const std::string&, int) {});
        lua.set_function("__profileEndEvent", [](const std::string&, const std::string&, int) {});

        // Execution policy mock
        lua.set_function("__setExecutionPolicy", [](bool) {});
    }

    /**
     * Reset orchestrator state for test isolation.
     * Call this in tests that need a clean slate after previous operations.
     */
    void resetOrchestrator()
    {
        lua.script(R"(
            Context.orchestrator = {
                sequences = {},
                scope = nil,
                solution = {},
                values = {},
                order_requirements = {},
                sync_requirements = {},
                enable_list = {}
            }
        )");
        teamFailCount = 0;
        teamSyncCount = 0;
        teamSyncStatuses.clear();
    }

    /**
     * Set the current stage.
     */
    void setStage(const std::string& stage)
    {
        lua.script("Context.info.stage = Stage." + stage);
    }

    /**
     * Helper: create a sequence and optionally set the scope to be inside it.
     * Useful for tests that need to operate within a sequence context.
     */
    void createSequence(const std::string& name, bool enterScope = false)
    {
        lua.script(
            "Orchestrator.CreateSequence('" + name + "', function() end, 'test', 0)");
        if (enterScope)
        {
            lua.script(
                "Context.orchestrator.scope = require('lua/core/framework/scope'):New('" + name + "')");
        }
    }

    /**
     * Helper: create a test within a sequence.
     * Creates the sequence first if it doesn't exist.
     */
    void createTest(const std::string& sequenceName, const std::string& testName, bool enterScope = false)
    {
        // Ensure the sequence exists
        lua.script(
            "if not Orchestrator.HasSequence(require('lua/core/framework/scope'):New('" + sequenceName + "')) then "
            "Orchestrator.CreateSequence('" + sequenceName + "', function() end, 'test', 0) end");
        // Set scope to the sequence
        lua.script(
            "Context.orchestrator.scope = require('lua/core/framework/scope'):New('" + sequenceName + "')");
        // Create the test
        lua.script(
            "Orchestrator.CreateTest('" + testName + "', function() end, 'test', 0)");
        if (enterScope)
        {
            lua.script(
                "Context.orchestrator.scope = require('lua/core/framework/scope'):New('" +
                sequenceName + "', '" + testName + "')");
        }
    }
};

#endif // FRASY_TESTS_ORCHESTRATOR_TEST_FIXTURE_H
