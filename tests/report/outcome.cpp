#include "utils/lua/orchestrator/orchestrator.h"
#include "utils/report/result.h"
#include "utils/UutState.h"

#include <gtest/gtest.h>

#include <vector>

using Frasy::UutState;
using Frasy::Report::Result;
using Orchestrator = Frasy::Lua::Orchestrator;

// =============================================================================
// Orchestrator::getReportOutcome — state and folder of a finished board
// =============================================================================

TEST(ReportOutcome, Pass_GoesToPass)
{
    const auto outcome = Orchestrator::getReportOutcome(Result::Pass, false);
    EXPECT_EQ(outcome.state, UutState::Passed);
    EXPECT_EQ(outcome.subdirectory, Orchestrator::passSubdirectory);
}

TEST(ReportOutcome, Fail_GoesToFail)
{
    const auto outcome = Orchestrator::getReportOutcome(Result::Fail, true);
    EXPECT_EQ(outcome.state, UutState::Failed);
    EXPECT_EQ(outcome.subdirectory, Orchestrator::failSubdirectory);
}

TEST(ReportOutcome, IncompleteWithoutFailure_IsIncompletePassedInDebug)
{
    const auto outcome = Orchestrator::getReportOutcome(Result::Incomplete, false);
    EXPECT_EQ(outcome.state, UutState::IncompletePassed);
    EXPECT_EQ(outcome.subdirectory, Orchestrator::debugSubdirectory);
    EXPECT_EQ(Orchestrator::debugSubdirectory, "debug");
}

TEST(ReportOutcome, IncompleteWithFailure_IsIncompleteFailedInDebug)
{
    const auto outcome = Orchestrator::getReportOutcome(Result::Incomplete, true);
    EXPECT_EQ(outcome.state, UutState::IncompleteFailed);
    EXPECT_EQ(outcome.subdirectory, Orchestrator::debugSubdirectory);
}

TEST(ReportOutcome, UnexpectedInfoResult_IsIncompleteNeverPassed)
{
    // `info` is never skipped or disabled; such values are filed as incomplete, never as passed.
    for (auto result : {Result::Skipped, Result::Disabled, static_cast<Result>(42)}) {
        EXPECT_EQ(Orchestrator::getReportOutcome(result, false).state, UutState::IncompletePassed);
        EXPECT_EQ(Orchestrator::getReportOutcome(result, true).state, UutState::IncompleteFailed);
        EXPECT_EQ(Orchestrator::getReportOutcome(result, false).subdirectory, Orchestrator::debugSubdirectory);
    }
}

// =============================================================================
// toString(UutState)
// =============================================================================

TEST(UutStateName, EveryStateHasAName)
{
    EXPECT_EQ(Frasy::toString(UutState::Disabled), "disabled");
    EXPECT_EQ(Frasy::toString(UutState::Idle), "idle");
    EXPECT_EQ(Frasy::toString(UutState::Waiting), "waiting");
    EXPECT_EQ(Frasy::toString(UutState::Running), "running");
    EXPECT_EQ(Frasy::toString(UutState::Passed), "passed");
    EXPECT_EQ(Frasy::toString(UutState::Failed), "failed");
    EXPECT_EQ(Frasy::toString(UutState::Error), "error");
    EXPECT_EQ(Frasy::toString(UutState::IncompletePassed), "incomplete_passed");
    EXPECT_EQ(Frasy::toString(UutState::IncompleteFailed), "incomplete_failed");
    EXPECT_EQ(Frasy::toString(static_cast<UutState>(42)), "unknown");
}

// =============================================================================
// RunVerdict — severity of a UUT and of a run
// =============================================================================

using Frasy::RunVerdict;

TEST(RunVerdict, VerdictOfEachState)
{
    EXPECT_EQ(Frasy::getVerdict(UutState::Passed), RunVerdict::Passed);
    EXPECT_EQ(Frasy::getVerdict(UutState::IncompletePassed), RunVerdict::Incomplete);
    EXPECT_EQ(Frasy::getVerdict(UutState::Failed), RunVerdict::Failed);
    EXPECT_EQ(Frasy::getVerdict(UutState::IncompleteFailed), RunVerdict::Failed);
    EXPECT_EQ(Frasy::getVerdict(UutState::Error), RunVerdict::Error);
    for (auto state : {UutState::Disabled, UutState::Idle, UutState::Waiting, UutState::Running}) {
        EXPECT_FALSE(Frasy::getVerdict(state).has_value()) << Frasy::toString(state);
    }
}

TEST(RunVerdict, MostSevereWins)
{
    using enum UutState;
    EXPECT_EQ(Frasy::getRunVerdict(std::vector {Passed, Passed}), RunVerdict::Passed);
    EXPECT_EQ(Frasy::getRunVerdict(std::vector {Passed, IncompletePassed}), RunVerdict::Incomplete);
    EXPECT_EQ(Frasy::getRunVerdict(std::vector {IncompletePassed, IncompleteFailed}), RunVerdict::Failed);
    EXPECT_EQ(Frasy::getRunVerdict(std::vector {IncompleteFailed, IncompletePassed}), RunVerdict::Failed);
    EXPECT_EQ(Frasy::getRunVerdict(std::vector {Failed, Error, IncompletePassed}), RunVerdict::Error);
}

TEST(RunVerdict, StatesWithoutResultAreIgnored)
{
    using enum UutState;
    EXPECT_EQ(Frasy::getRunVerdict(std::vector {Disabled, Passed}), RunVerdict::Passed);
    EXPECT_EQ(Frasy::getRunVerdict(std::vector {Disabled, Idle}), RunVerdict::Passed);
    EXPECT_EQ(Frasy::getRunVerdict(std::vector<UutState> {}), RunVerdict::Passed);
}

TEST(RunVerdict, Names)
{
    EXPECT_EQ(Frasy::toString(RunVerdict::Passed), "passed");
    EXPECT_EQ(Frasy::toString(RunVerdict::Incomplete), "incomplete");
    EXPECT_EQ(Frasy::toString(RunVerdict::Failed), "failed");
    EXPECT_EQ(Frasy::toString(RunVerdict::Error), "error");
}
