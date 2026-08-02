#include "orchestrator_test_fixture.h"

#include <gtest/gtest.h>

// =============================================================================
// IsInSequence / IsInTest
// =============================================================================

TEST_F(OrchestratorTestFixture, IsInSequence_FalseInitially)
{
    lua.script("Context.orchestrator.scope = nil");
    EXPECT_FALSE(lua.script("return Orchestrator.IsInSequence()").get<bool>());
}

TEST_F(OrchestratorTestFixture, IsInSequence_TrueWhenInSequence)
{
    createSequence("Seq", true);
    EXPECT_TRUE(lua.script("return Orchestrator.IsInSequence()").get<bool>());
}

TEST_F(OrchestratorTestFixture, IsInTest_FalseWhenOnlyInSequence)
{
    createSequence("Seq", true);
    EXPECT_FALSE(lua.script("return Orchestrator.IsInTest()").get<bool>());
}

TEST_F(OrchestratorTestFixture, IsInTest_TrueWhenInTest)
{
    createTest("Seq", "T1", true);
    EXPECT_TRUE(lua.script("return Orchestrator.IsInTest()").get<bool>());
}

TEST_F(OrchestratorTestFixture, IsInTest_FalseInitially)
{
    lua.script("Context.orchestrator.scope = nil");
    EXPECT_FALSE(lua.script("return Orchestrator.IsInTest()").get<bool>());
}

// =============================================================================
// HasSequence / HasTest / HasScope
// =============================================================================

TEST_F(OrchestratorTestFixture, HasSequence_FalseForNonExistent)
{
    EXPECT_FALSE(lua.script(
        "return Orchestrator.HasSequence(require('lua/core/framework/scope'):New('NoSuchSeq'))").get<bool>());
}

TEST_F(OrchestratorTestFixture, HasSequence_TrueAfterCreation)
{
    createSequence("Seq");
    EXPECT_TRUE(lua.script(
        "return Orchestrator.HasSequence(require('lua/core/framework/scope'):New('Seq'))").get<bool>());
}

TEST_F(OrchestratorTestFixture, HasTest_FalseForNonExistent)
{
    createSequence("Seq");
    EXPECT_FALSE(lua.script(
        "return Orchestrator.HasTest(require('lua/core/framework/scope'):New('Seq', 'NoSuchTest'))").get<bool>());
}

TEST_F(OrchestratorTestFixture, HasTest_TrueAfterCreation)
{
    createTest("Seq", "T1");
    EXPECT_TRUE(lua.script(
        "return Orchestrator.HasTest(require('lua/core/framework/scope'):New('Seq', 'T1'))").get<bool>());
}

TEST_F(OrchestratorTestFixture, HasScope_SequenceOnly)
{
    createSequence("Seq");
    EXPECT_TRUE(lua.script(
        "return Orchestrator.HasScope(require('lua/core/framework/scope'):New('Seq'))").get<bool>());
    EXPECT_FALSE(lua.script(
        "return Orchestrator.HasScope(require('lua/core/framework/scope'):New('Nope'))").get<bool>());
}

TEST_F(OrchestratorTestFixture, HasScope_SequenceAndTest)
{
    createTest("Seq", "T1");
    EXPECT_TRUE(lua.script(
        "return Orchestrator.HasScope(require('lua/core/framework/scope'):New('Seq', 'T1'))").get<bool>());
    EXPECT_FALSE(lua.script(
        "return Orchestrator.HasScope(require('lua/core/framework/scope'):New('Seq', 'T2'))").get<bool>());
}
