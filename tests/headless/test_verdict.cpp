#include "utils/headless/headless_runner.h"

#include <gtest/gtest.h>

using Frasy::RunVerdict;
using Frasy::UutState;

// =============================================================================
// Headless labels and exit code
// =============================================================================

TEST(HeadlessVerdict, ExitCodes)
{
    EXPECT_EQ(Frasy::Headless::getExitCode(RunVerdict::Passed), 0);
    EXPECT_EQ(Frasy::Headless::getExitCode(RunVerdict::Failed), 1);
    EXPECT_EQ(Frasy::Headless::getExitCode(RunVerdict::Error), 2);
    EXPECT_EQ(Frasy::Headless::getExitCode(RunVerdict::Incomplete), 3);
}

TEST(HeadlessVerdict, ExitCodeOfMixedRuns)
{
    using enum UutState;
    const auto exitCode = [](std::vector<UutState> states) {
        return Frasy::Headless::getExitCode(Frasy::getRunVerdict(states));
    };
    EXPECT_EQ(exitCode({Passed, IncompletePassed}), 3);
    EXPECT_EQ(exitCode({IncompletePassed, IncompleteFailed}), 1);
    EXPECT_EQ(exitCode({IncompletePassed, Error}), 2);
    EXPECT_EQ(exitCode({Passed, Passed}), 0);
}

TEST(HeadlessVerdict, StateLabels)
{
    EXPECT_EQ(Frasy::Headless::getStateLabel(UutState::Passed), "PASS");
    EXPECT_EQ(Frasy::Headless::getStateLabel(UutState::Failed), "FAIL");
    EXPECT_EQ(Frasy::Headless::getStateLabel(UutState::Error), "ERROR");
    EXPECT_EQ(Frasy::Headless::getStateLabel(UutState::IncompletePassed), "INCOMPLETE_PASS");
    EXPECT_EQ(Frasy::Headless::getStateLabel(UutState::IncompleteFailed), "INCOMPLETE_FAIL");
    EXPECT_EQ(Frasy::Headless::getStateLabel(UutState::Idle), "UNKNOWN");
}

TEST(HeadlessVerdict, VerdictLabels)
{
    EXPECT_EQ(Frasy::Headless::getVerdictLabel(RunVerdict::Passed), "PASS");
    EXPECT_EQ(Frasy::Headless::getVerdictLabel(RunVerdict::Incomplete), "INCOMPLETE");
    EXPECT_EQ(Frasy::Headless::getVerdictLabel(RunVerdict::Failed), "FAIL");
    EXPECT_EQ(Frasy::Headless::getVerdictLabel(RunVerdict::Error), "ERROR");
}
