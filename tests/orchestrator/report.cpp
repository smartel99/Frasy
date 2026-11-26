#include "execution_fixture.h"
#include "utils/report/result.h"

#include <gtest/gtest.h>

// =============================================================================
// CompileExecutionResults — v2 layout: info.result and format_version
// =============================================================================

class ReportTest : public ExecutionTest {
protected:
    void SetUp() override
    {
        ExecutionTest::SetUp();
        lua.script(R"(
            Context.map = { ibs = {}, reportFormat = 2, onReport = function(report) seen = report end }
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

    bool infoPass() { return lua.script("return saved.info.result == 'pass'").get<bool>(); }

    bool has(const std::string& field) { return lua.script("return " + field + " ~= nil").get<bool>(); }
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

TEST_F(ReportTest, DisabledSequenceAndFailure_Incomplete)
{
    lua.script(
      "Define({ { name = 'A', tests = { { name = 'T1', func = Fail } } }, "
      "{ name = 'B', tests = { { name = 'T1' } } } })");
    lua.script("Context.orchestrator.enable_list = { B = { enabled = false } }");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    // Anything disabled makes the board a debug run, even with a failure.
    EXPECT_EQ(info(), "incomplete");
    EXPECT_FALSE(infoPass());
}

TEST_F(ReportTest, IncompleteSequenceAndFailure_Incomplete)
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
    EXPECT_EQ(lua.script("return saved.sequences.B.result").get<std::string>(), "fail");
    EXPECT_EQ(info(), "incomplete");
    EXPECT_FALSE(infoPass());
}

TEST_F(ReportTest, DisabledTestAndFailureInSameSequence_SequenceFailsBoardIncomplete)
{
    lua.script("Define({ { name = 'A', tests = { { name = 'T1' }, { name = 'T2', func = Fail } } } })");
    lua.script(
      "Context.orchestrator.enable_list = { A = { enabled = true, T1 = { enabled = false }, "
      "T2 = { enabled = true } } }");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    // The sequence keeps showing its failure; the board is still a debug run.
    EXPECT_EQ(lua.script("return saved.sequences.A.result").get<std::string>(), "fail");
    EXPECT_EQ(info(), "incomplete");
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

// =============================================================================
// Sequences that don't run still report the tests the solution lists
// =============================================================================

TEST_F(ReportTest, DisabledSequence_ReportsListedTests)
{
    lua.script("Define({ { name = 'A', tests = { { name = 'T1' } } }, { name = 'B', tests = { { name = 'T1' } } } })");
    // A disabled sequence never runs its body, so its tests are never declared.
    lua.script("Context.orchestrator.sequences.B.tests = {}");
    lua.script("Context.orchestrator.enable_list = { B = { enabled = false } }");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_EQ(lua.script("return saved.sequences.B.tests.T1.result").get<std::string>(), "disabled");
    EXPECT_EQ(lua.script("return saved.sequences.B.tests.T1.reason").get<std::string>(), "Disabled");
    EXPECT_EQ(info(), "incomplete");
}

TEST_F(ReportTest, SkippedSequence_ReportsTestsDeclaredAfterRequires)
{
    lua.script(R"(Define({ { name = 'A', func = function() error(UnmetRequirement('no')) end,
                             tests = { { name = 'T1' }, { name = 'T2', section = 2 } } } }))");
    lua.script("Context.orchestrator.sequences.A.tests = {}");
    ASSERT_TRUE(runSection(1)) << lastError;
    ASSERT_TRUE(runSection(2)) << lastError;
    compile();
    for (const auto* test : {"T1", "T2"}) {
        EXPECT_EQ(lua.script(std::string("return saved.sequences.A.tests.") + test + ".result").get<std::string>(),
                  "skipped")
          << test;
    }
    EXPECT_EQ(info(), "pass");
}

TEST_F(ReportTest, DisabledSequence_SolutionFromJson)
{
    lua.script("Define({ { name = 'A', tests = { { name = 'T1' } } }, { name = 'B', tests = { { name = 'T1' } } } })");
    lua.script(R"(
        local Json = require('lua/core/vendor/json')
        Context.orchestrator.solution = Json.decode(Json.encode(Context.orchestrator.solution))
        Context.orchestrator.sequences.B.tests = {}
        Context.orchestrator.enable_list = { B = { enabled = false } }
    )");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_EQ(lua.script("return saved.sequences.B.tests.T1.result").get<std::string>(), "disabled");
}

TEST_F(ReportTest, ListedTest_ValuesAndRequirementsWork)
{
    lua.script("Scope = require('lua/core/framework/scope')");
    lua.script(R"(Define({
        { name = 'A', tests = { { name = 'T1' } } },
        { name = 'B', func = function()
            hasValue = Orchestrator.HasValue(Scope:New('A', 'T1'), 'x')
            if not Orchestrator.HasPassed(Scope:New('A', 'T1')) then error(UnmetRequirement('A.T1')) end
        end, tests = { { name = 'T1' } } },
    }))");
    lua.script("Context.orchestrator.sequences.A.tests = {}");
    lua.script("Context.orchestrator.enable_list = { A = { enabled = false } }");
    // A's listed test exists: B finds no value in it, and is skipped by its unmet requirement, as the execution
    // `ToPass()` requirement does.
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_FALSE(lua.script("return hasValue").get<bool>());
    EXPECT_EQ(result("B"), "skipped");
}

TEST_F(ReportTest, SequenceBodyRaises_ListedTestsFailWithIt)
{
    lua.script(R"(Define({
        { name = 'A', tests = { { name = 'T1' } } },
        { name = 'Broken', func = function() error('fixture not found') end,
          tests = { { name = 'Declared' }, { name = 'Never' }, { name = 'Later', section = 2 } } },
    }))");
    // The body raises after declaring `Declared`, before `Never` and `Later`.
    lua.script(
      "Context.orchestrator.sequences.Broken.tests.Never = nil; "
      "Context.orchestrator.sequences.Broken.tests.Later = nil");
    EXPECT_FALSE(runSection());
    compile();
    EXPECT_EQ(lua.script("return saved.sequences.Broken.result").get<std::string>(), "fail");
    // The traceback stays on the sequence; its tests get the first line.
    EXPECT_NE(lua.script("return saved.sequences.Broken.reason").get<std::string>().find("stack traceback:"),
              std::string::npos);
    for (const auto* test : {"Declared", "Never", "Later"}) {
        const auto path = std::string("saved.sequences.Broken.tests.") + test;
        EXPECT_EQ(lua.script("return " + path + ".result").get<std::string>(), "fail") << test;
        const auto testReason = lua.script("return " + path + ".reason").get<std::string>();
        EXPECT_NE(testReason.find("fixture not found"), std::string::npos) << test;
        EXPECT_EQ(testReason.find('\n'), std::string::npos) << test;
        EXPECT_EQ(testReason.find('\r'), std::string::npos) << test;
    }
    // Other sequences are left alone.
    EXPECT_EQ(lua.script("return saved.sequences.A.tests.T1.result").get<std::string>(), "pass");
    EXPECT_EQ(info(), "fail");
}

TEST_F(ReportTest, SkippedSequence_DeclaredTestsKept)
{
    lua.script(R"(Define({ { name = 'A', func = function() error(UnmetRequirement('no')) end,
                             tests = { { name = 'T1' }, { name = 'T2' } } } }))");
    lua.script("Context.orchestrator.sequences.A.tests.T2 = nil; declared = Context.orchestrator.sequences.A.tests.T1");
    ASSERT_TRUE(runSection()) << lastError;
    EXPECT_TRUE(lua.script("return Context.orchestrator.sequences.A.tests.T1 == declared").get<bool>());
    EXPECT_EQ(result("A", "T1"), "skipped");
    EXPECT_EQ(result("A", "T2"), "skipped");
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
    for (const auto* field : {"saved.info.pass",
                              "saved.sequences.A.pass",
                              "saved.sequences.A.tests.T1.enabled",
                              "saved.sequences.A.tests.T1.skipped",
                              "saved.sequences.A.tests.T2.pass"}) {
        EXPECT_FALSE(has(field)) << field;
    }
}

TEST_F(ReportTest, Expectations_ResultOnly)
{
    lua.script("Define({ { name = 'A', tests = { { name = 'T1', func = Fail } } } })");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_EQ(lua.script("return saved.sequences.A.tests.T1.expectations[1].result").get<std::string>(), "fail");
    EXPECT_FALSE(has("saved.sequences.A.tests.T1.expectations[1].pass"));
    // The orchestrator's own entries are untouched.
    EXPECT_FALSE(lua.script("return Context.orchestrator.sequences.A.tests.T1.expectations[1].pass").get<bool>());
    EXPECT_FALSE(has("Context.orchestrator.sequences.A.result.tests"));
}

TEST_F(ReportTest, OnReportTimeMutation_InternalUntouched)
{
    lua.script("Context.map.onReport = function(report) report.sequences.A.tests.T1.time.stop = -1 end");
    lua.script("Define({ { name = 'A', tests = { { name = 'T1' } } } })");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_EQ(lua.script("return saved.sequences.A.tests.T1.time.stop").get<double>(), -1.0);
    EXPECT_NE(lua.script("return Context.orchestrator.sequences.A.tests.T1.result.time.stop").get<double>(), -1.0);
}

TEST_F(ReportTest, OnReport_SeesResultAndVersion)
{
    lua.script("Define({ { name = 'A', tests = { { name = 'T1', func = Fail } } } })");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_EQ(info("seen"), "fail");
    EXPECT_EQ(lua.script("return seen.info.format_version").get<int>(), Frasy::Report::s_currentFormatVersion);
    EXPECT_FALSE(has("seen.info.pass"));
}

TEST_F(ReportTest, ErrorInLaterSection_FailedTestStillFailsBoard)
{
    // T2 runs in section 2 and raises: RunSequence never re-aggregates S, which keeps its section-1 result.
    lua.script(
      "Define({ { name = 'S', tests = { { name = 'T1' }, { name = 'T2', section = 2, "
      "func = function() error('boom') end } } } })");
    ASSERT_TRUE(runSection(1)) << lastError;
    EXPECT_FALSE(runSection(2));
    compile();
    EXPECT_EQ(lua.script("return saved.sequences.S.tests.T2.result").get<std::string>(), "fail");
    EXPECT_EQ(info(), "fail");
    EXPECT_FALSE(infoPass());
}

TEST_F(ReportTest, ErrorInLaterSectionWithDisabledTest_Incomplete)
{
    lua.script(
      "Define({ { name = 'S', tests = { { name = 'T1' }, { name = 'T2', section = 2, "
      "func = function() error('boom') end } } } })");
    lua.script(
      "Context.orchestrator.enable_list = { S = { enabled = true, T1 = { enabled = false }, "
      "T2 = { enabled = true } } }");
    ASSERT_TRUE(runSection(1)) << lastError;
    EXPECT_FALSE(runSection(2));
    compile();
    EXPECT_EQ(info(), "incomplete");
}

// =============================================================================
// CompileExecutionResults — v1 layout, the default
// =============================================================================

class ReportV1Test : public ReportTest {
protected:
    void SetUp() override
    {
        ReportTest::SetUp();
        lua.script("Context.map.reportFormat = nil");
    }

    bool savedPass() { return lua.script("return saved.info.pass").get<bool>(); }
};

TEST_F(ReportV1Test, NoFormat_BooleansOnly)
{
    lua.script("Define({ { name = 'A', tests = { { name = 'T1', func = Fail }, { name = 'T2' } } } })");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_FALSE(savedPass());
    EXPECT_FALSE(lua.script("return seen.info.pass").get<bool>());
    for (const auto* field : {"saved.info.result",
                              "saved.info.format_version",
                              "saved.sequences.A.result",
                              "saved.sequences.A.tests.T1.result",
                              "saved.sequences.A.tests.T1.expectations[1].result"}) {
        EXPECT_FALSE(has(field)) << field;
    }
    EXPECT_FALSE(lua.script("return saved.sequences.A.pass").get<bool>());
    EXPECT_TRUE(lua.script("return saved.sequences.A.enabled").get<bool>());
    EXPECT_FALSE(lua.script("return saved.sequences.A.tests.T1.expectations[1].pass").get<bool>());
    EXPECT_TRUE(lua.script("return saved.sequences.A.tests.T2.pass").get<bool>());
    // Internal results keep `result`.
    EXPECT_EQ(result("A", "T1"), "fail");
}

TEST_F(ReportV1Test, ExplicitV1_SameAsDefault)
{
    lua.script("Context.map.reportFormat = 1");
    lua.script("Define({ { name = 'A', tests = { { name = 'T1' } } } })");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_TRUE(savedPass());
    EXPECT_FALSE(has("saved.info.result"));
}

TEST_F(ReportV1Test, Incomplete_InfoPassFalse)
{
    lua.script("Define({ { name = 'A', tests = { { name = 'T1' }, { name = 'T2' } } } })");
    lua.script(
      "Context.orchestrator.enable_list = { A = { enabled = true, T1 = { enabled = false }, "
      "T2 = { enabled = true } } }");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_FALSE(savedPass());
    EXPECT_FALSE(lua.script("return saved.sequences.A.tests.T1.enabled").get<bool>());
    EXPECT_TRUE(lua.script("return saved.sequences.A.tests.T1.skipped").get<bool>());
}

TEST_F(ReportV1Test, HandBuiltExpectationWithResultOnly_GetsPass)
{
    lua.script(R"(Define({ { name = 'A', tests = { { name = 'T1', func = function()
        Orchestrator.AddExpectationResult({ name = 'E', result = 'pass' })
    end } } } }))");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    EXPECT_TRUE(lua.script("return saved.sequences.A.tests.T1.expectations[1].pass").get<bool>());
    EXPECT_FALSE(has("saved.sequences.A.tests.T1.expectations[1].result"));
}

// =============================================================================
// Environment.SetReportFormat
// =============================================================================

class ReportFormatTest : public OrchestratorTestFixture {
protected:
    void SetUp() override
    {
        OrchestratorTestFixture::SetUp();
        lua.script("Context.map = nil; require('lua/core/sdk/environment/environment')");
    }
};

TEST_F(ReportFormatTest, Default_V1)
{
    EXPECT_EQ(lua.script("return Context.map.reportFormat").get<int>(), 1);
    EXPECT_EQ(lua.script("return ReportFormat.v1").get<int>(), 1);
    EXPECT_EQ(lua.script("return ReportFormat.v2").get<int>(), 2);
}

TEST_F(ReportFormatTest, SetV2)
{
    lua.script("Environment.SetReportFormat(ReportFormat.v2)");
    EXPECT_EQ(lua.script("return Context.map.reportFormat").get<int>(), 2);
}

TEST_F(ReportFormatTest, Unknown_Errors)
{
    for (const auto* format : {"3", "'v2'", "nil", "0"}) {
        const auto run =
          lua.safe_script(std::string("Environment.SetReportFormat(") + format + ")", sol::script_pass_on_error);
        EXPECT_FALSE(run.valid()) << format;
    }
    EXPECT_EQ(lua.script("return Context.map.reportFormat").get<int>(), 1);
}

TEST_F(ReportV1Test, ListedTests_Counted)
{
    lua.script("Define({ { name = 'A', tests = { { name = 'T1' } } }, { name = 'B', tests = { { name = 'T1' } } } })");
    lua.script("Context.orchestrator.sequences.B.tests = {}");
    lua.script("Context.orchestrator.enable_list = { B = { enabled = false } }");
    ASSERT_TRUE(runSection()) << lastError;
    compile();
    const auto report =
      nlohmann::json::parse(lua.script("return require('lua/core/vendor/json').encode(saved)").get<std::string>());
    const auto counts = Frasy::Report::countTests(report);
    EXPECT_EQ(counts.get(Frasy::Report::Result::Pass), 1U);
    EXPECT_EQ(counts.get(Frasy::Report::Result::Disabled), 1U);
    EXPECT_EQ(Frasy::Report::ResultReader(report).getInfo(), Frasy::Report::Result::Incomplete);
}

TEST_F(ReportV1Test, SequenceBodyRaises_ListedTestsRead)
{
    lua.script(R"(Define({
        { name = 'A', tests = { { name = 'T1' } } },
        { name = 'Broken', func = function() error('fixture not found') end, tests = { { name = 'Never' } } },
    }))");
    lua.script("Context.orchestrator.sequences.Broken.tests = {}");
    EXPECT_FALSE(runSection());
    compile();
    const auto report =
      nlohmann::json::parse(lua.script("return require('lua/core/vendor/json').encode(saved)").get<std::string>());
    const Frasy::Report::ResultReader reader(report);
    EXPECT_EQ(reader.getInfo(), Frasy::Report::Result::Fail);
    EXPECT_EQ(reader.read(report.at("sequences").at("Broken")), Frasy::Report::Result::Fail);
    EXPECT_TRUE(Frasy::Report::hasFailedScope(report));
    const auto counts = Frasy::Report::countTests(report);
    EXPECT_EQ(counts.get(Frasy::Report::Result::Pass), 1U);
    EXPECT_EQ(counts.get(Frasy::Report::Result::Fail), 1U);
}
