/**
 * @file    solution.cpp
 * @brief   Unit tests for Models::Solution: ordering, generation parsing, enable flags and per-UUT states.
 */
#include <gtest/gtest.h>
#include <utils/models/solution.h>

#include <initializer_list>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using Frasy::Models::ExecutionState;
using Frasy::Models::Sequence;
using Frasy::Models::Solution;

namespace {
using Names = std::vector<std::string>;

Solution FromJson(const std::string& text)
{
    std::istringstream is(text);
    return Solution::FromGeneration(is);
}

// Two sections, "A" is split across both (subsequences), with two test stages in "B".
constexpr auto s_splitGeneration = R"([
    [[ {"name": "A", "tests": [["a1", "a2"]]},
       {"name": "B", "tests": [["b1"], ["b2", "b3"]]} ]],
    [[ {"name": "A", "tests": [["a3"]]} ],
     [ {"name": "C", "tests": [["c1"]]} ]]
])";

// Invalid JSON, or valid JSON with a malformed entry, possibly after valid ones.
constexpr const char* s_badGenerations[] = {
  R"([[[ {"name": "Z", "tests": [["z1"]] )",                                       // Truncated.
  R"([[[ {"tests": [["z1"]]} ]]])",                                                // Missing name.
  R"([[[ {"name": "Z"} ]]])",                                                      // Missing tests.
  R"([[[ {"name": "Z", "tests": [[42]]} ]]])",                                     // Test is not a string.
  R"([[[ {"name": "A", "tests": [["x"]]}, {"name": "Z"} ]]])",                     // Valid prefix, then missing tests.
  R"([[[ {"name": "A", "tests": [["x"]]} ]], [[ {"name": 7, "tests": []} ]]])",    // Bad second section.
};

Solution Replaced(Solution solution, const std::string& text, std::size_t uutCount)
{
    solution.ReplaceWith(FromJson(text), uutCount);
    return solution;
}
}    // namespace

// =============================================================================
// Construction / ordering
// =============================================================================

TEST(Solution, NewEntriesAreEnabled)
{
    Solution solution;
    solution.AddTest("S", "T");
    EXPECT_TRUE(solution.sequences.at("S").enabled);
    EXPECT_TRUE(solution.sequences.at("S").tests.at("T").enabled);
}

TEST(Solution, AddSequenceKeepsFirstAppearanceOrder)
{
    Solution solution;
    solution.AddSequence("B");
    solution.AddSequence("A");
    solution.AddSequence("B");
    EXPECT_EQ(solution.sequenceOrder, (Names {"B", "A"}));
    EXPECT_EQ(solution.sequences.size(), 2u);
}

TEST(Solution, AddSequenceReturnsExistingEntry)
{
    Solution solution;
    solution.AddSequence("A").enabled = false;
    EXPECT_FALSE(solution.AddSequence("A").enabled);
}

TEST(Solution, AddTestCreatesSequenceAndDedupes)
{
    Solution solution;
    solution.AddTest("S", "T2");
    solution.AddTest("S", "T1");
    solution.AddTest("S", "T2");
    EXPECT_EQ(solution.sequenceOrder, (Names {"S"}));
    EXPECT_EQ(solution.sequences.at("S").testOrder, (Names {"T2", "T1"}));
    EXPECT_EQ(solution.sequences.at("S").tests.size(), 2u);
}

TEST(Solution, SequenceConstructorDedupesTests)
{
    Sequence sequence(false, {"T1", "T2", "T1"});
    EXPECT_FALSE(sequence.enabled);
    EXPECT_EQ(sequence.testOrder, (Names {"T1", "T2"}));
    EXPECT_EQ(sequence.tests.size(), 2u);
    EXPECT_TRUE(sequence.tests.at("T1").enabled);
}

TEST(Solution, ClearResetsOrder)
{
    Solution solution = FromJson(s_splitGeneration);
    solution.Clear();
    EXPECT_TRUE(solution.sections.empty());
    EXPECT_TRUE(solution.sequences.empty());
    EXPECT_TRUE(solution.sequenceOrder.empty());
}

TEST(Solution, SetEnableOnUnknownThrows)
{
    Solution solution;
    solution.AddTest("S", "T");
    EXPECT_THROW(solution.SetSequenceEnable("X", false), std::out_of_range);
    EXPECT_THROW(solution.SetTestEnable("S", "X", false), std::out_of_range);
    EXPECT_THROW(solution.SetTestEnable("X", "T", false), std::out_of_range);
}

// =============================================================================
// FromGeneration
// =============================================================================

TEST(SolutionFromGeneration, KeepsSectionsLayout)
{
    const Solution solution = FromJson(s_splitGeneration);
    ASSERT_EQ(solution.sections.size(), 2u);
    ASSERT_EQ(solution.sections[0].size(), 1u);
    ASSERT_EQ(solution.sections[0][0].size(), 2u);
    EXPECT_EQ(solution.sections[0][0][1].first, "B");
    EXPECT_EQ(solution.sections[0][0][1].second, (std::vector<Names> {{"b1"}, {"b2", "b3"}}));
    ASSERT_EQ(solution.sections[1].size(), 2u);
    EXPECT_EQ(solution.sections[1][0][0].first, "A");
    EXPECT_EQ(solution.sections[1][1][0].first, "C");
}

TEST(SolutionFromGeneration, SequenceOrderIsFirstAppearance)
{
    const Solution solution = FromJson(s_splitGeneration);
    EXPECT_EQ(solution.sequenceOrder, (Names {"A", "B", "C"}));
    EXPECT_EQ(solution.sequences.size(), 3u);
}

TEST(SolutionFromGeneration, TestOrderSpansStagesAndSubsequences)
{
    const Solution solution = FromJson(s_splitGeneration);
    EXPECT_EQ(solution.sequences.at("A").testOrder, (Names {"a1", "a2", "a3"}));
    EXPECT_EQ(solution.sequences.at("B").testOrder, (Names {"b1", "b2", "b3"}));
    EXPECT_EQ(solution.sequences.at("C").testOrder, (Names {"c1"}));
}

TEST(SolutionFromGeneration, EntriesEnabledAndStatesEmpty)
{
    const Solution solution = FromJson(s_splitGeneration);
    for (const auto& [sName, sequence] : solution.sequences) {
        EXPECT_TRUE(sequence.enabled) << sName;
        EXPECT_TRUE(sequence.state.empty()) << sName;
        for (const auto& [tName, test] : sequence.tests) {
            EXPECT_TRUE(test.enabled) << tName;
            EXPECT_TRUE(test.state.empty()) << tName;
        }
    }
}

TEST(SolutionFromGeneration, EmptyGeneration)
{
    const Solution solution = FromJson("[]");
    EXPECT_TRUE(solution.sections.empty());
    EXPECT_TRUE(solution.sequences.empty());
    EXPECT_TRUE(solution.sequenceOrder.empty());
}

TEST(SolutionFromGeneration, BadInputThrows)
{
    for (const auto* text : s_badGenerations) {
        EXPECT_ANY_THROW(FromJson(text)) << text;
    }
}

// =============================================================================
// ReplaceWith
// =============================================================================

TEST(SolutionReplaceWith, KeepsEnableFlags)
{
    Solution solution = FromJson(s_splitGeneration);
    solution.SetSequenceEnable("B", false);
    solution.SetTestEnable("A", "a3", false);

    // "C" removed, "D" added, "a4" added.
    solution = Replaced(solution,
                        R"([[[ {"name": "A", "tests": [["a1", "a3", "a4"]]},
                               {"name": "B", "tests": [["b1"]]},
                               {"name": "D", "tests": [["d1"]]} ]]])",
                        1);

    EXPECT_EQ(solution.sequenceOrder, (Names {"A", "B", "D"}));
    EXPECT_FALSE(solution.sequences.contains("C"));
    EXPECT_FALSE(solution.sequences.at("B").enabled);
    EXPECT_TRUE(solution.sequences.at("B").tests.at("b1").enabled);
    EXPECT_TRUE(solution.sequences.at("A").enabled);
    EXPECT_FALSE(solution.sequences.at("A").tests.at("a3").enabled);
    EXPECT_TRUE(solution.sequences.at("A").tests.at("a4").enabled);
    EXPECT_TRUE(solution.sequences.at("D").enabled);
}

TEST(SolutionReplaceWith, RemovedThenReaddedEntryIsEnabledAgain)
{
    Solution solution = FromJson(s_splitGeneration);
    solution.SetSequenceEnable("C", false);
    solution.SetTestEnable("A", "a3", false);

    solution.ReplaceWith(FromJson(R"([[[ {"name": "A", "tests": [["a1"]]} ]]])"), 1);
    ASSERT_FALSE(solution.sequences.contains("C"));
    solution.ReplaceWith(FromJson(s_splitGeneration), 1);

    EXPECT_TRUE(solution.sequences.at("C").enabled);
    EXPECT_TRUE(solution.sequences.at("A").tests.at("a3").enabled);
}

TEST(SolutionReplaceWith, ResetsStates)
{
    Solution solution = FromJson(s_splitGeneration);
    solution.SetSequenceEnable("B", false);
    solution.ResetStates(1);
    solution.sequences.at("A").state[1] = ExecutionState::failed;

    solution.ReplaceWith(FromJson(s_splitGeneration), 2);

    const auto& a = solution.sequences.at("A");
    EXPECT_EQ(a.state, (std::vector {ExecutionState::idle, ExecutionState::idle, ExecutionState::idle}));
    EXPECT_EQ(a.tests.at("a3").state.size(), 3u);
    EXPECT_EQ(solution.sequences.at("B").state[2], ExecutionState::disabled);
    EXPECT_EQ(solution.sequences.at("B").tests.at("b2").state[1], ExecutionState::disabled);
}

TEST(SolutionReplaceWith, BadGenerationLeavesSolutionUntouched)
{
    Solution solution = FromJson(s_splitGeneration);
    solution.SetSequenceEnable("B", false);
    solution.SetTestEnable("A", "a2", false);
    solution.ResetStates(2);
    solution.sequences.at("C").state[1] = ExecutionState::passed;

    for (const auto* text : s_badGenerations) {
        EXPECT_ANY_THROW(solution.ReplaceWith(FromJson(text), 4)) << text;
        EXPECT_EQ(solution.sections.size(), 2u) << text;
        EXPECT_EQ(solution.sequenceOrder, (Names {"A", "B", "C"})) << text;
        EXPECT_EQ(solution.sequences.at("A").testOrder, (Names {"a1", "a2", "a3"})) << text;
        EXPECT_FALSE(solution.sequences.contains("Z")) << text;
        EXPECT_FALSE(solution.sequences.at("B").enabled) << text;
        EXPECT_FALSE(solution.sequences.at("A").tests.at("a2").enabled) << text;
        EXPECT_EQ(solution.sequences.at("B").state.size(), 3u) << text;
        EXPECT_EQ(solution.sequences.at("C").state[1], ExecutionState::passed) << text;
    }
}

// =============================================================================
// ResetStates
// =============================================================================

TEST(SolutionResetStates, SizesForOneBasedUuts)
{
    Solution solution = FromJson(s_splitGeneration);
    solution.ResetStates(3);
    for (const auto& [sName, sequence] : solution.sequences) {
        EXPECT_EQ(sequence.state.size(), 4u) << sName;
        for (const auto& [tName, test] : sequence.tests) {
            EXPECT_EQ(test.state.size(), 4u) << tName;
        }
    }
}

TEST(SolutionResetStates, NoUuts)
{
    Solution solution = FromJson(s_splitGeneration);
    solution.ResetStates(0);
    EXPECT_EQ(solution.sequences.at("A").state.size(), 1u);
    EXPECT_EQ(solution.sequences.at("A").tests.at("a1").state.size(), 1u);
}

TEST(SolutionResetStates, IdleOrDisabledFromEnableFlags)
{
    Solution solution = FromJson(s_splitGeneration);
    solution.SetSequenceEnable("B", false);
    solution.SetTestEnable("A", "a2", false);
    solution.ResetStates(2);

    const auto& a = solution.sequences.at("A");
    const auto& b = solution.sequences.at("B");
    EXPECT_EQ(a.state, (std::vector {ExecutionState::idle, ExecutionState::idle, ExecutionState::idle}));
    EXPECT_EQ(a.tests.at("a1").state[1], ExecutionState::idle);
    EXPECT_EQ(a.tests.at("a2").state[1], ExecutionState::disabled);
    EXPECT_EQ(b.state[2], ExecutionState::disabled);
    // Tests of a disabled sequence are disabled, whatever their own flag.
    EXPECT_TRUE(b.tests.at("b1").enabled);
    EXPECT_EQ(b.tests.at("b1").state[2], ExecutionState::disabled);
}

TEST(SolutionResetStates, OverwritesPreviousRun)
{
    Solution solution = FromJson(s_splitGeneration);
    solution.ResetStates(1);
    auto& a                   = solution.sequences.at("A");
    a.state[1]                = ExecutionState::failed;
    a.tests.at("a1").state[1] = ExecutionState::running;

    solution.ResetStates(2);
    EXPECT_EQ(a.state, (std::vector {ExecutionState::idle, ExecutionState::idle, ExecutionState::idle}));
    EXPECT_EQ(a.tests.at("a1").state[1], ExecutionState::idle);
}

// =============================================================================
// State transitions
// =============================================================================

namespace {
using enum ExecutionState;

Solution Running(std::size_t uutCount = 2)
{
    Solution solution = FromJson(s_splitGeneration);
    solution.ResetStates(uutCount);
    return solution;
}
}    // namespace

TEST(SolutionStates, NormalTestLifecycle)
{
    Solution solution = Running();
    EXPECT_TRUE(solution.SetTestState(1, "A", "a1", running));
    EXPECT_EQ(solution.sequences.at("A").tests.at("a1").state[1], running);
    EXPECT_EQ(solution.sequences.at("A").tests.at("a1").state[2], idle);    // Other UUT untouched.
    EXPECT_TRUE(solution.SetTestState(1, "A", "a1", passed));
    EXPECT_EQ(solution.sequences.at("A").tests.at("a1").state[1], passed);
    EXPECT_FALSE(solution.SetTestState(1, "A", "a1", passed));    // Unchanged.
}

TEST(SolutionStates, EndNeverOverwritesSkipOrDisable)
{
    for (const auto skip : {disabled, skipped}) {
        Solution solution = Running();
        ASSERT_TRUE(solution.SetTestState(1, "A", "a1", skip));
        ASSERT_TRUE(solution.SetSequenceState(1, "B", skip));
        for (const auto next : {running, passed, failed, error}) {
            EXPECT_FALSE(solution.SetTestState(1, "A", "a1", next)) << int(skip) << " -> " << int(next);
            EXPECT_FALSE(solution.SetSequenceState(1, "B", next)) << int(skip) << " -> " << int(next);
        }
        EXPECT_EQ(solution.sequences.at("A").tests.at("a1").state[1], skip);
        EXPECT_EQ(solution.sequences.at("B").state[1], skip);
    }
}

TEST(SolutionStates, SkipAndDisableAlwaysApply)
{
    Solution solution = Running();
    ASSERT_TRUE(solution.SetTestState(1, "A", "a1", running));
    EXPECT_TRUE(solution.SetTestState(1, "A", "a1", skipped));
    EXPECT_TRUE(solution.SetTestState(1, "A", "a1", disabled));
    EXPECT_EQ(solution.sequences.at("A").tests.at("a1").state[1], disabled);
}

TEST(SolutionStates, StartDoesntOverwriteFlagDisabled)
{
    Solution solution = FromJson(s_splitGeneration);
    solution.SetTestEnable("A", "a2", false);
    solution.ResetStates(1);
    EXPECT_FALSE(solution.SetTestState(1, "A", "a2", running));
    EXPECT_FALSE(solution.SetTestState(1, "A", "a2", passed));
    EXPECT_EQ(solution.sequences.at("A").tests.at("a2").state[1], disabled);
}

TEST(SolutionStates, SequenceSkipCascadesToIdleTestsOnly)
{
    Solution solution = Running();
    ASSERT_TRUE(solution.SetTestState(1, "A", "a1", passed));
    ASSERT_TRUE(solution.SetSequenceState(1, "A", skipped));
    const auto& a = solution.sequences.at("A");
    EXPECT_EQ(a.state[1], skipped);
    EXPECT_EQ(a.tests.at("a1").state[1], passed);    // Already done, kept.
    EXPECT_EQ(a.tests.at("a2").state[1], skipped);
    EXPECT_EQ(a.tests.at("a3").state[1], skipped);
    EXPECT_EQ(a.tests.at("a2").state[2], idle);    // Other UUT untouched.
}

TEST(SolutionStates, RepeatedSequenceSkipIsNoChange)
{
    Solution solution = Running();
    ASSERT_TRUE(solution.SetSequenceState(1, "B", disabled));
    EXPECT_FALSE(solution.SetSequenceState(1, "B", disabled));
    EXPECT_EQ(solution.sequences.at("B").tests.at("b3").state[1], disabled);
}

TEST(SolutionStates, UnknownTargetsIgnored)
{
    Solution solution = Running(1);
    EXPECT_FALSE(solution.SetSequenceState(1, "Z", running));
    EXPECT_FALSE(solution.SetTestState(1, "Z", "a1", running));
    EXPECT_FALSE(solution.SetTestState(1, "A", "z", running));
    EXPECT_FALSE(solution.SetSequenceState(2, "A", running));    // UUT out of range.
    EXPECT_FALSE(solution.SetTestState(2, "A", "a1", running));
    EXPECT_FALSE(solution.SetSequenceState(99, "A", skipped));

    Solution empty = FromJson(s_splitGeneration);    // States never sized.
    EXPECT_FALSE(empty.SetSequenceState(0, "A", running));
    EXPECT_FALSE(empty.SetTestState(0, "A", "a1", running));
}

// =============================================================================
// ApplyProgress
// =============================================================================

TEST(SolutionApplyProgress, MapsEveryEventType)
{
    struct Case {
        const char*    type;
        bool           pass;
        bool           isTest;
        ExecutionState expected;
    };
    for (const auto& [type, pass, isTest, expected] : std::initializer_list<Case> {
           {"sequence_start", true, false, running},
           {"sequence_end", true, false, passed},
           {"sequence_end", false, false, failed},
           {"sequence_disabled", true, false, disabled},
           {"sequence_skipped", false, false, skipped},
           {"test_start", true, true, running},
           {"test_end", true, true, passed},
           {"test_end", false, true, failed},
           {"test_disabled", true, true, disabled},
           {"test_skipped", true, true, skipped},
         }) {
        Solution solution = Running();
        if (isTest) {
            EXPECT_TRUE(solution.ApplyProgress(type, 2, "b2", "B", pass)) << type;
            EXPECT_EQ(solution.sequences.at("B").tests.at("b2").state[2], expected) << type << " " << pass;
            EXPECT_EQ(solution.sequences.at("B").state[2], idle) << type;
        }
        else {
            EXPECT_TRUE(solution.ApplyProgress(type, 2, "B", "", pass)) << type;
            EXPECT_EQ(solution.sequences.at("B").state[2], expected) << type << " " << pass;
        }
        EXPECT_EQ(solution.sequences.at("B").state[1], idle) << type;    // Other UUT untouched.
    }
}

TEST(SolutionApplyProgress, IgnoresExpectationsAndUnknownTypes)
{
    Solution solution = Running();
    EXPECT_FALSE(solution.ApplyProgress("expectation", 1, "Voltage", "A", false));
    EXPECT_FALSE(solution.ApplyProgress("", 1, "A", "", true));
    EXPECT_FALSE(solution.ApplyProgress("Sequence_Start", 1, "A", "", true));
    EXPECT_FALSE(solution.ApplyProgress("test_start", 1, "a1", "Z", true));    // Unknown parent.
    for (const auto& [sName, sequence] : solution.sequences) {
        EXPECT_EQ(sequence.state, (std::vector {idle, idle, idle})) << sName;
    }
}

TEST(SolutionApplyProgress, LuaDisabledTestSequence)
{
    // What Lua reports for a disabled test: start, disabled, then end with pass = true.
    Solution solution = Running();
    EXPECT_TRUE(solution.ApplyProgress("test_start", 1, "a2", "A", true));
    EXPECT_TRUE(solution.ApplyProgress("test_disabled", 1, "a2", "A", true));
    EXPECT_FALSE(solution.ApplyProgress("test_end", 1, "a2", "A", true));
    EXPECT_EQ(solution.sequences.at("A").tests.at("a2").state[1], disabled);
}

// =============================================================================
// DisableUut / FinishRun
// =============================================================================

TEST(SolutionStates, DisableUut)
{
    Solution solution = Running();
    solution.DisableUut(2);
    for (const auto& [sName, sequence] : solution.sequences) {
        EXPECT_EQ(sequence.state[1], idle) << sName;
        EXPECT_EQ(sequence.state[2], disabled) << sName;
        for (const auto& [tName, test] : sequence.tests) {
            EXPECT_EQ(test.state[1], idle) << tName;
            EXPECT_EQ(test.state[2], disabled) << tName;
        }
    }
    solution.DisableUut(99);    // Out of range: ignored.
}

TEST(SolutionStates, FinishRunAbortsOnlyRunning)
{
    Solution solution = Running();
    solution.SetSequenceState(1, "A", running);
    solution.SetTestState(1, "A", "a1", passed);
    solution.SetTestState(1, "A", "a2", running);
    solution.SetTestState(2, "A", "a1", failed);
    solution.SetSequenceState(2, "B", skipped);

    solution.FinishRun();

    const auto& a = solution.sequences.at("A");
    EXPECT_EQ(a.state[1], error);
    EXPECT_EQ(a.tests.at("a1").state[1], passed);
    EXPECT_EQ(a.tests.at("a2").state[1], error);
    EXPECT_EQ(a.tests.at("a3").state[1], idle);    // Never reached.
    EXPECT_EQ(a.tests.at("a1").state[2], failed);
    EXPECT_EQ(solution.sequences.at("B").state[2], skipped);
    EXPECT_EQ(solution.sequences.at("C").state[1], idle);
}
