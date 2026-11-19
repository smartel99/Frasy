/**
 * @file    progress_events.cpp
 * @brief   Events reported through __progress.report by RunSequence / RunTest, for every outcome.
 */
#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {
/// "type name pass", e.g. "test_end T1 true".
using Events = std::vector<std::string>;
}    // namespace

class ProgressEventsTest : public OrchestratorTestFixture {
protected:
    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        lua.open_libraries(sol::lib::os);    // ExecuteSection / RunTest time with os.clock().
        // Record every event but expectations, which are out of scope for these ordering tests.
        // The fixture's Team.Fail is registered as a global named "Team.Fail", not as a field: define it here.
        // ScopeRequirement keeps its generation implementation in this fixture (picked when first required), so
        // ToPass() isn't checked when executing: unmet requirements use an always-false RuntimeRequirement.
        lua.script(R"(
            Team.Fail = function() end
            Unmet = function()
                if Context.info.stage == Stage.execution then
                    local RuntimeRequirement = require("lua/core/framework/runtime_requirement")
                    Requires(RuntimeRequirement:New(function() return false end, "Unmet on purpose"))
                end
            end
            __events = {}
            __progress = {
                report = function(type, uut, name, parentA, parentB, pass)
                    if type ~= "expectation" then
                        table.insert(__events, string.format("%s %s %s", type, name, tostring(pass)))
                    end
                end
            }
        )");
    }

    /**
     * Generate, then execute every section as UUT 1.
     * @return the error that stopped the run (re-raised by RunSequence), empty if none
     */
    std::string run(const std::string& enableList = "{}")
    {
        lua.script("Orchestrator.Generate()");
        lua.script("Context.orchestrator.enable_list = " + enableList);
        return lua
          .script(R"(
            Context.info.stage = Stage.execution
            Context.info.uut = 1
            Context.info.time = Context.info.time or {}
            for index = 1, #Context.orchestrator.solution do
                local ok, err = pcall(Orchestrator.ExecuteSection, index)
                if not ok then return tostring(type(err) == "table" and err.what or err) end
            end
            return ""
        )")
          .get<std::string>();
    }

    Events events() { return lua["__events"].get<Events>(); }
};

TEST_F(ProgressEventsTest, PassAndFail)
{
    lua.script(R"(
        Sequence("Seq", function()
            Test("T1", function() Expect(1, "One"):ToBeEqual(1) end)
            Test("T2", function()
                Requires(Test("T1"):ToBeFirst())
                Expect(false, "Falsehood"):ToBeTrue()
            end)
        end)
    )");
    EXPECT_EQ(run(), "");
    EXPECT_EQ(events(),
              (Events {
                "sequence_start Seq true",
                "test_start T1 true",
                "test_end T1 true",
                "test_start T2 true",
                "test_end T2 false",
                "sequence_end Seq false",
              }));
}

TEST_F(ProgressEventsTest, TestSkippedByRequirement)
{
    lua.script(R"(
        Sequence("Seq", function()
            Test("T1", function() Expect(false, "Falsehood"):ToBeTrue() end)
            Test("T2", function()
                Requires(Test("T1"):ToBeFirst())
                Unmet()
            end)
        end)
    )");
    EXPECT_EQ(run(), "");
    EXPECT_EQ(events(),
              (Events {
                "sequence_start Seq true",
                "test_start T1 true",
                "test_end T1 false",
                "test_start T2 true",
                "test_skipped T2 true",
                "test_end T2 true",
                "sequence_end Seq false",
              }));
}

TEST_F(ProgressEventsTest, TestDisabled)
{
    lua.script(R"(
        Sequence("Seq", function()
            Test("T1", function() end)
            Test("T2", function() Requires(Test("T1"):ToPass()) end)
        end)
    )");
    EXPECT_EQ(run("{ Seq = { enabled = true, T1 = { enabled = true }, T2 = { enabled = false } } }"), "");
    EXPECT_EQ(events(),
              (Events {
                "sequence_start Seq true",
                "test_start T1 true",
                "test_end T1 true",
                "test_start T2 true",
                "test_disabled T2 true",
                "test_end T2 true",
                "sequence_end Seq false",
              }));
}

TEST_F(ProgressEventsTest, SequenceSkippedByRequirement)
{
    lua.script(R"(
        Sequence("A", function()
            Test("A1", function() Expect(false, "Falsehood"):ToBeTrue() end)
        end)
        Sequence("B", function()
            Requires(Sequence("A"):ToBeFirst())
            Unmet()
            Test("B1", function() end)
        end)
    )");
    EXPECT_EQ(run(), "");
    // B reports its start, then skipped, and never an end nor anything about its tests.
    EXPECT_EQ(events(),
              (Events {
                "sequence_start A true",
                "test_start A1 true",
                "test_end A1 false",
                "sequence_end A false",
                "sequence_start B true",
                "sequence_skipped B true",
              }));
}

TEST_F(ProgressEventsTest, SequenceDisabled)
{
    lua.script(R"(
        Sequence("A", function()
            Test("A1", function() end)
        end)
        Sequence("B", function()
            Requires(Sequence("A"):ToPass())
            Test("B1", function() end)
        end)
    )");
    EXPECT_EQ(run("{ B = { enabled = false } }"), "");
    EXPECT_EQ(events(),
              (Events {
                "sequence_start A true",
                "test_start A1 true",
                "test_end A1 true",
                "sequence_end A true",
                "sequence_start B true",
                "sequence_disabled B true",
              }));
}

TEST_F(ProgressEventsTest, AbortReportsNoEnd)
{
    // Dropped from Phase 4 on purpose: no test_end before a re-raised error, the C++ side turns it into error.
    lua.script(R"(
        Sequence("Seq", function()
            Test("T1", function() Expect(false, "Abort", { policy = ErrorPolicy.stopAll }):ToBeTrue() end)
        end)
    )");
    // Aborted by the expectation itself, not by a missing mock.
    const auto error = run();
    EXPECT_TRUE(error.starts_with("UnmetExpectation")) << error;
    EXPECT_EQ(events(), (Events {"sequence_start Seq true", "test_start T1 true"}));
}

// =============================================================================
// Sequence split across sections: RunSequence is entered once per section
// =============================================================================

class ProgressEventsSplitTest : public ProgressEventsTest {
protected:
    void SetUp() override
    {
        ProgressEventsTest::SetUp();
        // Sync() puts T1 in a later section than T2, so "Split" is entered once per section.
        lua.script(R"(
            Sequence("Split", function()
                SplitBody()
                Test("T1", function() Sync() end)
                Test("T2", function() end)
            end)
            Sequence("Other", function()
                Test("O1", function() end)
            end)
        )");
    }

    /// Number of sections in which "Split" has a subsequence.
    int splitSections()
    {
        return lua.script(R"(
            local count = 0
            for _, section in ipairs(Context.orchestrator.solution) do
                local found = false
                for _, stage in ipairs(section) do
                    for _, sq in ipairs(stage) do found = found or sq.name == "Split" end
                end
                if found then count = count + 1 end
            end
            return count
        )");
    }

    /// Recorded events about "Split" or its tests.
    Events splitEvents()
    {
        Events result;
        for (const auto& event : events()) {
            if (event.find(" Split ") != std::string::npos || event.find(" T1 ") != std::string::npos ||
                event.find(" T2 ") != std::string::npos) {
                result.push_back(event);
            }
        }
        return result;
    }
};

TEST_F(ProgressEventsSplitTest, SkippedReportedOnce)
{
    lua.script("SplitBody = function() Unmet() end");
    EXPECT_EQ(run(), "");
    ASSERT_GE(splitSections(), 2);
    EXPECT_EQ(splitEvents(), (Events {"sequence_start Split true", "sequence_skipped Split true"}));
}

TEST_F(ProgressEventsSplitTest, DisabledReportedOnce)
{
    lua.script("SplitBody = function() end");
    EXPECT_EQ(run("{ Split = { enabled = false } }"), "");
    ASSERT_GE(splitSections(), 2);
    EXPECT_EQ(splitEvents(), (Events {"sequence_start Split true", "sequence_disabled Split true"}));
}
