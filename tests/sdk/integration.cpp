#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

class SdkIntegrationTest : public OrchestratorTestFixture {};

TEST_F(SdkIntegrationTest, FullScenario_GenerationProducesSolution)
{
    // Define a realistic test script
    lua.script(R"(
        Sequence("Init", function()
            Requires(Sequence():ToBeFirst())
            Test("PowerOn", function() end)
            Test("WaitStable", function()
                Requires(Test("PowerOn"):ToPass())
            end)
        end)

        Sequence("Functional", function()
            Requires(Sequence("Init"):ToPass())
            Test("CheckVoltage", function() end)
            Test("CheckCurrent", function() end)
        end)

        Sequence("Cleanup", function()
            Requires(Sequence():ToBeLast())
            Test("PowerOff", function() end)
        end)
    )");

    // Generate the solution
    lua.script("Orchestrator.Generate()");

    // Verify solution structure
    auto solutionSize = lua.script("return #Context.orchestrator.solution").get<int>();
    EXPECT_GT(solutionSize, 0);

    // Verify ordering: Init before Functional before Cleanup
    lua.script(R"(
        __order = {}
        for _, section in ipairs(Context.orchestrator.solution) do
            for _, stage in ipairs(section) do
                for _, seq in ipairs(stage) do
                    table.insert(__order, seq.name)
                end
            end
        end
    )");

    auto initPos = lua.script("for i, n in ipairs(__order) do if n == 'Init' then return i end end; return -1").get<int>();
    auto funcPos = lua.script("for i, n in ipairs(__order) do if n == 'Functional' then return i end end; return -1").get<int>();
    auto cleanPos = lua.script("for i, n in ipairs(__order) do if n == 'Cleanup' then return i end end; return -1").get<int>();

    EXPECT_LT(initPos, funcPos);
    EXPECT_LT(funcPos, cleanPos);

    // Verify test ordering within Init: PowerOn before WaitStable
    lua.script(R"(
        __test_order = {}
        for _, section in ipairs(Context.orchestrator.solution) do
            for _, stage in ipairs(section) do
                for _, seq in ipairs(stage) do
                    if seq.name == 'Init' then
                        for _, tstage in ipairs(seq.tests) do
                            for _, t in ipairs(tstage) do
                                table.insert(__test_order, t)
                            end
                        end
                    end
                end
            end
        end
    )");

    auto powerOnPos = lua.script("for i, n in ipairs(__test_order) do if n == 'PowerOn' then return i end end; return -1").get<int>();
    auto waitPos = lua.script("for i, n in ipairs(__test_order) do if n == 'WaitStable' then return i end end; return -1").get<int>();
    EXPECT_LT(powerOnPos, waitPos);
}

TEST_F(SdkIntegrationTest, FullScenario_WithSync)
{
    lua.script(R"(
        Sequence("SeqA", function()
            Test("T1", function()
                Sync()
            end)
            Test("T2", function() end)
        end)
        Sequence("SeqB", function()
            Test("T1", function() end)
        end)
    )");

    lua.script("Orchestrator.Generate()");

    // Sync should cause multiple sections
    auto sections = lua.script("return #Context.orchestrator.solution").get<int>();
    EXPECT_GT(sections, 1);

    // Verify both sequences are present in the solution
    lua.script(R"(
        __seq_names = {}
        __test_names = {}
        for _, section in ipairs(Context.orchestrator.solution) do
            for _, stage in ipairs(section) do
                for _, seq in ipairs(stage) do
                    table.insert(__seq_names, seq.name)
                    if __test_names[seq.name] == nil then
                        __test_names[seq.name] = {}
                    end
                    for _, tstage in ipairs(seq.tests) do
                        for _, t in ipairs(tstage) do
                            table.insert(__test_names[seq.name], t)
                        end
                    end
                end
            end
        end
    )");

    // Both sequences should appear
    EXPECT_TRUE(lua.script(R"(
        for _, n in ipairs(__seq_names) do if n == 'SeqA' then return true end end
        return false
    )").get<bool>());
    EXPECT_TRUE(lua.script(R"(
        for _, n in ipairs(__seq_names) do if n == 'SeqB' then return true end end
        return false
    )").get<bool>());

    // SeqA's T1 (synced) should be in a separate section from T2
    // Verify T1 and T2 are both present for SeqA
    EXPECT_TRUE(lua.script(R"(
        for _, t in ipairs(__test_names['SeqA']) do if t == 'T1' then return true end end
        return false
    )").get<bool>());
    EXPECT_TRUE(lua.script(R"(
        for _, t in ipairs(__test_names['SeqA']) do if t == 'T2' then return true end end
        return false
    )").get<bool>());

    // SeqB's T1 should be present
    EXPECT_TRUE(lua.script(R"(
        for _, t in ipairs(__test_names['SeqB']) do if t == 'T1' then return true end end
        return false
    )").get<bool>());
}
