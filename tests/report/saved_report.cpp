#include "orchestrator/execution_fixture.h"
#include "utils/lua/orchestrator/orchestrator.h"
#include "utils/lua/save_as_json.h"
#include "utils/report/result.h"

#include <gtest/gtest.h>
#include <json.hpp>

#include <filesystem>
#include <fstream>

using Frasy::Report::Result;
using Frasy::Report::ResultReader;

// =============================================================================
// Reports written by the real orchestrator and SaveAsJson, read back with ResultReader
// =============================================================================

class SavedReportTest : public ExecutionTest {
protected:
    std::filesystem::path m_dir;

    void SetUp() override
    {
        ExecutionTest::SetUp();
        lua.set_function("SaveAsJson", &Frasy::Lua::SaveAsJson);
        lua.script(R"(
            Context.map = { ibs = {}, onReport = function(report) end }
            Context.info.version = { frasy = '1', orchestrator = '1', scripts = '1', application = '1' }
            Context.info.title = 'Demo'
        )");
        const auto* test = ::testing::UnitTest::GetInstance()->current_test_info();
        m_dir = std::filesystem::temp_directory_path() / "frasy_tests" / test->test_suite_name() / test->name();
        std::filesystem::remove_all(m_dir);
        std::filesystem::create_directories(m_dir);
    }

    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove_all(m_dir, ec);
        ExecutionTest::TearDown();
    }

    nlohmann::json compileAndLoad()
    {
        lua.script("Orchestrator.CompileExecutionResults('" + m_dir.generic_string() + "')");
        std::ifstream file(m_dir / "0.json");
        EXPECT_TRUE(file.is_open());
        return nlohmann::json::parse(file);
    }
};

TEST_F(SavedReportTest, VersionIsReadable)
{
    lua.script("Define({ { name = 'S', tests = { { name = 'T1' } } } })");
    ASSERT_TRUE(runSection()) << lastError;
    const auto         report = compileAndLoad();
    const ResultReader reader(report);
    EXPECT_EQ(reader.getFormatVersion(), Frasy::Report::s_currentFormatVersion);
    EXPECT_EQ(reader.getInfo(), Result::Pass);
}

TEST_F(SavedReportTest, EveryScopeResultRoundTrips)
{
    lua.script(R"(
        local Expectation = require('lua/core/framework/expectation/expectation')
        Define({
            { name = 'Partial', tests = { { name = 'Off' }, { name = 'On' } } },
            { name = 'Broken', tests = { { name = 'T1', func = function()
                Expectation:New(1, 'x', { inverted = true }):ToBeEqual(1)
                Expectation:New(1, 'y', { inverted = true }):ToBeEqual(2)
            end } } },
            { name = 'NotApplicable', func = function() error(UnmetRequirement('no')) end,
              tests = { { name = 'T1' } } },
        })
        Context.orchestrator.enable_list = { Partial = { enabled = true, Off = { enabled = false },
                                                         On = { enabled = true } } }
    )");
    ASSERT_TRUE(runSection()) << lastError;
    const auto         report = compileAndLoad();
    const ResultReader reader(report);
    const auto&        sequences = report.at("sequences");

    // A disabled test makes the board incomplete, even with a failure elsewhere.
    EXPECT_EQ(reader.getInfo(), Result::Incomplete);
    EXPECT_TRUE(Frasy::Report::hasFailedScope(report));

    const auto partial = reader.read(sequences.at("Partial"));
    EXPECT_EQ(partial, Result::Incomplete);
    EXPECT_EQ(reader.read(sequences.at("Partial").at("tests").at("Off"), partial), Result::Disabled);
    EXPECT_EQ(reader.read(sequences.at("Partial").at("tests").at("On"), partial), Result::Pass);

    const auto broken = reader.read(sequences.at("Broken"));
    EXPECT_EQ(broken, Result::Fail);
    const auto& expectations = sequences.at("Broken").at("tests").at("T1").at("expectations");
    ASSERT_EQ(expectations.size(), 2U);
    EXPECT_EQ(reader.read(expectations.at(0)), Result::Fail);
    EXPECT_EQ(reader.read(expectations.at(1)), Result::Pass);

    const auto notApplicable = reader.read(sequences.at("NotApplicable"));
    EXPECT_EQ(notApplicable, Result::Skipped);
    EXPECT_EQ(reader.read(sequences.at("NotApplicable").at("tests").at("T1"), notApplicable), Result::Skipped);
}

TEST_F(SavedReportTest, DeprecatedFieldsAgreeWithResult)
{
    lua.script("Define({ { name = 'S', tests = { { name = 'T1' }, { name = 'T2' } } } })");
    lua.script(
      "Context.orchestrator.enable_list = { S = { enabled = true, T1 = { enabled = false }, "
      "T2 = { enabled = true } } }");
    ASSERT_TRUE(runSection()) << lastError;
    const auto report = compileAndLoad();
    // A v1 reader of the same file still gets a consistent picture from the booleans.
    auto asV1 = report;
    asV1["info"].erase("format_version");
    const ResultReader v1(asV1);
    const ResultReader v2(report);
    const auto&        tests = report.at("sequences").at("S").at("tests");
    for (const auto* name : {"T1", "T2"}) {
        EXPECT_EQ(v1.read(tests.at(name)), v2.read(tests.at(name))) << name;
    }
    EXPECT_EQ(v2.getInfo(), Result::Incomplete);
    EXPECT_EQ(v1.getInfo(), Result::Fail);    // v1 had no incomplete: `info.pass` is false.
}

TEST_F(SavedReportTest, FailWithDisabledTest_IsIncompleteWithFailure)
{
    lua.script(R"(Define({
        { name = 'A', tests = { { name = 'T1', func = Fail } } },
        { name = 'B', tests = { { name = 'T1' }, { name = 'T2' } } },
    }))");
    lua.script(
      "Context.orchestrator.enable_list = { B = { enabled = true, T1 = { enabled = false }, "
      "T2 = { enabled = true } } }");
    ASSERT_TRUE(runSection()) << lastError;
    const auto         report = compileAndLoad();
    const ResultReader reader(report);
    EXPECT_EQ(reader.getInfo(), Result::Incomplete);
    EXPECT_TRUE(Frasy::Report::hasFailedScope(report));
    EXPECT_EQ(Frasy::Lua::Orchestrator::getReportOutcome(reader.getInfo(), Frasy::Report::hasFailedScope(report)).state,
              Frasy::UutState::IncompleteFailed);
}

TEST_F(SavedReportTest, DisabledTestOnly_IsIncompleteWithoutFailure)
{
    lua.script("Define({ { name = 'A', tests = { { name = 'T1' }, { name = 'T2' } } } })");
    lua.script(
      "Context.orchestrator.enable_list = { A = { enabled = true, T1 = { enabled = false }, "
      "T2 = { enabled = true } } }");
    ASSERT_TRUE(runSection()) << lastError;
    const auto         report = compileAndLoad();
    const ResultReader reader(report);
    EXPECT_EQ(reader.getInfo(), Result::Incomplete);
    EXPECT_FALSE(Frasy::Report::hasFailedScope(report));
    EXPECT_EQ(Frasy::Lua::Orchestrator::getReportOutcome(reader.getInfo(), Frasy::Report::hasFailedScope(report)).state,
              Frasy::UutState::IncompletePassed);
}

TEST_F(SavedReportTest, DisabledAndFailingTestInSameSequence_IsIncompleteWithFailure)
{
    lua.script("Define({ { name = 'A', tests = { { name = 'T1' }, { name = 'T2', func = Fail } } } })");
    lua.script(
      "Context.orchestrator.enable_list = { A = { enabled = true, T1 = { enabled = false }, "
      "T2 = { enabled = true } } }");
    ASSERT_TRUE(runSection()) << lastError;
    const auto         report = compileAndLoad();
    const ResultReader reader(report);
    EXPECT_EQ(reader.read(report.at("sequences").at("A")), Result::Fail);
    EXPECT_EQ(reader.getInfo(), Result::Incomplete);
    EXPECT_EQ(Frasy::Lua::Orchestrator::getReportOutcome(reader.getInfo(), Frasy::Report::hasFailedScope(report)).state,
              Frasy::UutState::IncompleteFailed);
}

TEST_F(SavedReportTest, ErrorInLaterSectionWithDisabledTest_IsIncompleteWithFailure)
{
    lua.script(
      "Define({ { name = 'S', tests = { { name = 'T1' }, { name = 'T2', section = 2, "
      "func = function() error('boom') end }, { name = 'T3' } } } })");
    lua.script(
      "Context.orchestrator.enable_list = { S = { enabled = true, T1 = { enabled = false }, "
      "T2 = { enabled = true }, T3 = { enabled = true } } }");
    ASSERT_TRUE(runSection(1)) << lastError;
    EXPECT_FALSE(runSection(2));
    const auto         report = compileAndLoad();
    const ResultReader reader(report);
    // S still reads its section-1 result; only its failed test shows the failure.
    EXPECT_NE(reader.read(report.at("sequences").at("S")), Result::Fail);
    EXPECT_EQ(reader.getInfo(), Result::Incomplete);
    EXPECT_TRUE(Frasy::Report::hasFailedScope(report));
    EXPECT_EQ(Frasy::Lua::Orchestrator::getReportOutcome(reader.getInfo(), Frasy::Report::hasFailedScope(report)).state,
              Frasy::UutState::IncompleteFailed);
}
