#include "execution_fixture.h"
#include "utils/report/result.h"

#include <gtest/gtest.h>

// =============================================================================
// CompileExecutionResults — info.result and format_version
// =============================================================================

class ReportTest : public ExecutionTest {
protected:
    void SetUp() override
    {
        ExecutionTest::SetUp();
        lua.script(R"(
            Context.map = { ibs = {}, onReport = function(report) seen = report end }
            Context.info.version = { frasy = '1', orchestrator = '1', scripts = '1', application = '1' }
            Context.info.title = 'Demo'
            function SaveAsJson(report, path) saved = report end
        )");
    }

    void compile() { lua.script("Orchestrator.CompileExecutionResults('out')"); }

    std::string info(const std::string& table = "saved")
    {
        return lua.script("return " + table + ".info.result").get<std::string>();
    }

    bool infoPass() { return lua.script("return saved.info.pass").get<bool>(); }
};

TEST_F(ReportTest, AllPass)
{
    lua.script("Define({ { name = 'A', tests = { { name = 'T1' } } }, { name = 'B', tests = { { name = 'T1' } } } })");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_EQ(info(), "pass");
    EXPECT_TRUE(infoPass());
    EXPECT_EQ(lua.script("return saved.info.format_version").get<int>(), Frasy::Report::s_currentFormatVersion);
}

TEST_F(ReportTest, OneFailure_Fails)
{
    lua.script(
      "Define({ { name = 'A', tests = { { name = 'T1', func = Fail } } }, "
      "{ name = 'B', tests = { { name = 'T1' } } } })");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_EQ(info(), "fail");
    EXPECT_FALSE(infoPass());
}

TEST_F(ReportTest, DisabledTest_Incomplete)
{
    lua.script("Define({ { name = 'A', tests = { { name = 'T1' }, { name = 'T2' } } } })");
    lua.script(
      "Context.orchestrator.enable_list = { A = { enabled = true, T1 = { enabled = false }, "
      "T2 = { enabled = true } } }");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_EQ(info(), "incomplete");
    EXPECT_FALSE(infoPass());
}

TEST_F(ReportTest, DisabledSequence_Incomplete)
{
    lua.script("Define({ { name = 'A', tests = { { name = 'T1' } } }, { name = 'B', tests = { { name = 'T1' } } } })");
    lua.script("Context.orchestrator.enable_list = { B = { enabled = false } }");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_EQ(info(), "incomplete");
    EXPECT_FALSE(infoPass());
}

TEST_F(ReportTest, DisabledAndFailure_Fails)
{
    lua.script(
      "Define({ { name = 'A', tests = { { name = 'T1', func = Fail } } }, "
      "{ name = 'B', tests = { { name = 'T1' } } } })");
    lua.script("Context.orchestrator.enable_list = { B = { enabled = false } }");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_EQ(info(), "fail");
    EXPECT_FALSE(infoPass());
}

TEST_F(ReportTest, IncompleteAndFailure_Fails)
{
    lua.script(
      "Define({ { name = 'A', tests = { { name = 'T1' }, { name = 'T2' } } }, "
      "{ name = 'B', tests = { { name = 'T1', func = Fail } } } })");
    lua.script(
      "Context.orchestrator.enable_list = { A = { enabled = true, T1 = { enabled = false }, "
      "T2 = { enabled = true } } }");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_EQ(lua.script("return saved.sequences.A.result").get<std::string>(), "incomplete");
    EXPECT_EQ(info(), "fail");
    EXPECT_FALSE(infoPass());
}

TEST_F(ReportTest, AllSequencesSkipped_Pass)
{
    lua.script(R"(Define({
        { name = 'A', func = function() error(UnmetRequirement('no')) end, tests = { { name = 'T1' } } },
        { name = 'B', func = function() error(UnmetRequirement('no')) end, tests = { { name = 'T1' } } },
    }))");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_EQ(info(), "pass");
    EXPECT_TRUE(infoPass());
}

TEST_F(ReportTest, NoSequence_Pass)
{
    lua.script("Context.info.time.start = 0");
    compile();
    EXPECT_EQ(info(), "pass");
    EXPECT_TRUE(infoPass());
    EXPECT_EQ(lua.script("return saved.info.format_version").get<int>(), Frasy::Report::s_currentFormatVersion);
}

TEST_F(ReportTest, OnReportError_StillSaved)
{
    lua.script("Context.map.onReport = function(report) error('hook broke') end");
    lua.script("Define({ { name = 'A', tests = { { name = 'T1', func = Fail } } } })");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_EQ(info(), "fail");
    EXPECT_FALSE(infoPass());
    EXPECT_EQ(lua.script("return saved.info.format_version").get<int>(), Frasy::Report::s_currentFormatVersion);
}

TEST_F(ReportTest, OnReportMutation_IsSaved)
{
    lua.script(R"(Context.map.onReport = function(report)
        report.sequences.A = nil
        report.info.result = 'pass'
        report.info.pass = true
    end)");
    lua.script(
      "Define({ { name = 'A', tests = { { name = 'T1', func = Fail } } }, "
      "{ name = 'B', tests = { { name = 'T1' } } } })");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_TRUE(lua.script("return saved.sequences.A == nil").get<bool>());
    EXPECT_EQ(info(), "pass");
    EXPECT_TRUE(infoPass());
}

TEST_F(ReportTest, SkippedSequence_IsNeutral)
{
    lua.script(R"(Define({
        { name = 'A', tests = { { name = 'T1' } } },
        { name = 'B', func = function() error(UnmetRequirement('no')) end, tests = { { name = 'T1' } } },
    }))");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_EQ(lua.script("return saved.sequences.B.result").get<std::string>(), "skipped");
    EXPECT_EQ(info(), "pass");
    EXPECT_TRUE(infoPass());
}

TEST_F(ReportTest, SequenceThatNeverRan_Fails)
{
    // Defined but left out of the solution, like a sequence cut short by an earlier error.
    lua.script("Define({ { name = 'A', tests = { { name = 'T1' } } } })");
    lua.script("Context.orchestrator.scope = nil; Orchestrator.CreateSequence('B', function() end, 'test', 0)");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_EQ(lua.script("return saved.sequences.B.result").get<std::string>(), "fail");
    EXPECT_EQ(info(), "fail");
}

TEST_F(ReportTest, ScopeResultsAreReported)
{
    lua.script("Define({ { name = 'A', tests = { { name = 'T1' }, { name = 'T2' } } } })");
    lua.script(
      "Context.orchestrator.enable_list = { A = { enabled = true, T1 = { enabled = false }, "
      "T2 = { enabled = true } } }");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_EQ(lua.script("return saved.sequences.A.result").get<std::string>(), "incomplete");
    EXPECT_EQ(lua.script("return saved.sequences.A.tests.T1.result").get<std::string>(), "disabled");
    EXPECT_EQ(lua.script("return saved.sequences.A.tests.T2.result").get<std::string>(), "pass");
    // Deprecated fields are still written during the transition.
    EXPECT_FALSE(lua.script("return saved.sequences.A.tests.T1.enabled").get<bool>());
    EXPECT_TRUE(lua.script("return saved.sequences.A.tests.T1.skipped").get<bool>());
    EXPECT_TRUE(lua.script("return saved.sequences.A.tests.T2.pass").get<bool>());
}

TEST_F(ReportTest, OnReport_SeesResultAndVersion)
{
    lua.script("Define({ { name = 'A', tests = { { name = 'T1', func = Fail } } } })");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_EQ(info("seen"), "fail");
    EXPECT_EQ(lua.script("return seen.info.format_version").get<int>(), Frasy::Report::s_currentFormatVersion);
    EXPECT_FALSE(lua.script("return seen.info.pass").get<bool>());
}
