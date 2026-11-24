#include "utils/report/formatters/formatter.h"
#include "utils/report/formatters/json.h"
#include "utils/report/formatters/keyValue.h"
#include "utils/report/formatters/markdown.h"
#include "utils/report/formatters/pdf.h"
#include "utils/report/json.h"
#include "utils/report/kvp.h"
#include "utils/report/pdf.h"
#include "utils/report/result.h"

#include "log_once.h"
#include <gtest/gtest.h>
#include <json.hpp>
#include <sol/sol.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

using Frasy::Report::Result;
namespace Formatter = Frasy::Report::Formatter;
namespace fs        = std::filesystem;

// =============================================================================
// Results read from in-process report tables, as the report writers see them
// =============================================================================

class ReportFormatterTest : public ::testing::Test {
protected:
    sol::state lua;
    fs::path   m_dir;

    static void SetUpTestSuite() { initLogOnce(); }

    void SetUp() override
    {
        lua.open_libraries(sol::lib::base);
        const auto* test = ::testing::UnitTest::GetInstance()->current_test_info();
        m_dir            = fs::temp_directory_path() / "frasy_tests" / test->test_suite_name() / test->name();
        fs::remove_all(m_dir);
        fs::create_directories(m_dir);
    }

    void TearDown() override
    {
        std::error_code ec;
        fs::remove_all(m_dir, ec);
    }

    sol::table table(const std::string& code) { return lua.script("return " + code).get<sol::table>(); }

    /** A report table shaped like CompileExecutionResults' output, with @p info and @p sequences spliced in. */
    sol::table report(const std::string& info, const std::string& sequences)
    {
        return table(R"({
            info = { date = 'today', title = 'Demo', operator = 'Op', serial = 'SN1', uut = 1,
                     time = { elapsed = 1.0 },
                     version = { frasy = '1', orchestrator = '1', scripts = '1', application = '1' }, )" +
                     info + R"( },
            ib = {},
            sequences = )" +
                     sequences + " }");
    }

    /** One sequence S with a failed test T, a disabled test D and a skipped test K. */
    sol::table mixedReport(const std::string& info)
    {
        return report(info, R"({ S = { result = 'fail', time = { elapsed = 1.0 }, tests = {
            T = { result = 'fail', time = { elapsed = 1.0 }, expectations = {
                { name = 'E', method = 'ToBeEqual', value = 1, expected = 2, result = 'fail', pass = false } } },
            D = { result = 'disabled', time = { elapsed = 0 }, expectations = {} },
            K = { result = 'skipped', time = { elapsed = 0 }, expectations = {} } } } })");
    }

    [[nodiscard]] static std::string read(const fs::path& file)
    {
        std::ifstream      in(file);
        std::ostringstream text;
        text << in.rdbuf();
        return text.str();
    }

    template<typename F>
    static void walk(F& formatter, const std::string& sequence, std::initializer_list<const char*> tests)
    {
        formatter.reportSequenceResult(sequence);
        for (const auto* test : tests) {
            formatter.reportTestResult(test);
            for (sol::table expectation = formatter.getNextExpectation(); !expectation.empty();
                 expectation            = formatter.getNextExpectation()) {
                formatter.reportExpectationResult(expectation);
            }
        }
    }

    std::string keyValue(const sol::table& results, std::initializer_list<const char*> tests)
    {
        const auto file = m_dir / "report.txt";
        {
            std::ofstream out(file);
            auto          formatter = Formatter::KeyValue(out, results);
            formatter.reportInfo();
            walk(formatter, "S", tests);
        }
        return read(file);
    }
};

TEST_F(ReportFormatterTest, GetResult_ReadsResult)
{
    EXPECT_EQ(Frasy::Report::getResult(table("{ result = 'incomplete' }")), Result::Incomplete);
    EXPECT_EQ(Frasy::Report::getResult(table("{ result = 'disabled', pass = true }")), Result::Disabled);
}

TEST_F(ReportFormatterTest, GetResult_FallsBackToBooleansLikeV1)
{
    EXPECT_EQ(Frasy::Report::getResult(table("{ pass = true }")), Result::Pass);
    EXPECT_EQ(Frasy::Report::getResult(table("{ pass = false }")), Result::Fail);
    EXPECT_EQ(Frasy::Report::getResult(table("{ skipped = true, pass = false }")), Result::Skipped);
    EXPECT_EQ(Frasy::Report::getResult(table("{ enabled = false, skipped = true, pass = false }")), Result::Disabled);
    EXPECT_EQ(Frasy::Report::getResult(table("{ result = 'bogus', pass = true }")), Result::Pass);
}

TEST_F(ReportFormatterTest, GetResult_NothingWithoutResultOrBooleans)
{
    EXPECT_FALSE(Frasy::Report::getResult(table("{}")).has_value());
    EXPECT_FALSE(Frasy::Report::getResult(table("{ result = 'bogus' }")).has_value());
    EXPECT_FALSE(Frasy::Report::getResult(sol::table {}).has_value());
}

TEST_F(ReportFormatterTest, IsIncomplete)
{
    EXPECT_TRUE(Formatter::isIncomplete(table("{ info = { result = 'incomplete', pass = false } }")));
    EXPECT_FALSE(Formatter::isIncomplete(table("{ info = { result = 'fail', pass = false } }")));
    EXPECT_FALSE(Formatter::isIncomplete(table("{ info = { pass = false } }")));
    EXPECT_FALSE(Formatter::isIncomplete(table("{}")));
}

TEST_F(ReportFormatterTest, Labels)
{
    EXPECT_EQ(Formatter::toLabel(Result::Pass), "PASS");
    EXPECT_EQ(Formatter::toLabel(Result::Fail), "FAIL");
    EXPECT_EQ(Formatter::toLabel(Result::Skipped), "SKIPPED");
    EXPECT_EQ(Formatter::toLabel(Result::Disabled), "DISABLED");
    EXPECT_EQ(Formatter::toLabel(Result::Incomplete), "INCOMPLETE");
}

TEST_F(ReportFormatterTest, KeyValue_PrintsEachResult)
{
    const auto text = keyValue(mixedReport("result = 'fail', pass = false"), {"T", "D", "K"});
    EXPECT_NE(text.find("\nInfo-Result: FAIL\n"), std::string::npos) << text;
    EXPECT_NE(text.find("\nS-Result: FAIL\n"), std::string::npos) << text;
    EXPECT_NE(text.find("\nS-T-Result: FAIL\n"), std::string::npos) << text;
    EXPECT_NE(text.find("\nS-T-1-Result: FAIL\n"), std::string::npos) << text;
    EXPECT_NE(text.find("\nS-D-Result: DISABLED\n"), std::string::npos) << text;
    EXPECT_NE(text.find("\nS-K-Result: SKIPPED\n"), std::string::npos) << text;
}

TEST_F(ReportFormatterTest, KeyValue_ResultWinsOverPass)
{
    const auto text = keyValue(report("result = 'pass', pass = false",
                                      R"({ S = { result = 'pass', pass = false, time = { elapsed = 0 }, tests = {
                 T = { result = 'pass', pass = false, time = { elapsed = 0 }, expectations = {} } } } })"),
                               {"T"});
    EXPECT_NE(text.find("\nInfo-Result: PASS\n"), std::string::npos) << text;
    EXPECT_NE(text.find("\nS-Result: PASS\n"), std::string::npos) << text;
    EXPECT_NE(text.find("\nS-T-Result: PASS\n"), std::string::npos) << text;
}

TEST_F(ReportFormatterTest, KeyValue_HandBuiltEntriesUseBooleans)
{
    const auto text = keyValue(report("pass = true",
                                      R"({ S = { pass = true, time = { elapsed = 0 }, tests = {
                 T = { pass = true, time = { elapsed = 0 }, expectations = {
                     { name = 'E', method = 'ToBeEqual', value = 1, expected = 1, pass = true } } },
                 K = { skipped = true, pass = false, time = { elapsed = 0 }, expectations = {} } } } })"),
                               {"T", "K"});
    EXPECT_NE(text.find("\nInfo-Result: PASS\n"), std::string::npos) << text;
    EXPECT_NE(text.find("\nS-T-1-Result: PASS\n"), std::string::npos) << text;
    EXPECT_NE(text.find("\nS-K-Result: SKIPPED\n"), std::string::npos) << text;
    EXPECT_EQ(text.find("Result: Not provided"), std::string::npos) << text;
}

TEST_F(ReportFormatterTest, Markdown_PrintsEachResult)
{
    const auto file = m_dir / "report.md";
    {
        std::ofstream out(file);
        auto          formatter = Formatter::Markdown(&out, mixedReport("result = 'fail', pass = false"));
        formatter.reportInfo();
        walk(formatter, "S", {"T", "D", "K"});
    }
    const auto text = read(file);
    EXPECT_NE(text.find("Result: FAIL"), std::string::npos) << text;
    EXPECT_NE(text.find("Result: DISABLED"), std::string::npos) << text;
    EXPECT_NE(text.find("Result: SKIPPED"), std::string::npos) << text;
    EXPECT_EQ(text.find("Result: Not provided"), std::string::npos) << text;
}

TEST_F(ReportFormatterTest, Json_AddsResultNextToDeprecatedFields)
{
    const auto file = m_dir / "report.json";
    {
        auto formatter = Formatter::Json(mixedReport("result = 'fail', pass = false"));
        formatter.reportInfo();
        walk(formatter, "S", {"T", "D", "K"});
        formatter.toFile(file.string());
    }
    const auto json = nlohmann::json::parse(read(file));
    EXPECT_EQ(json["info"]["result"], "fail");
    EXPECT_EQ(json["info"]["pass"], false);
    const auto& sequence = json["sequences"]["S"];
    EXPECT_EQ(sequence["result"], "fail");
    EXPECT_EQ(sequence["tests"]["T"]["result"], "fail");
    EXPECT_EQ(sequence["tests"]["D"]["result"], "disabled");
    EXPECT_EQ(sequence["tests"]["K"]["result"], "skipped");
    EXPECT_TRUE(sequence["tests"]["K"].contains("skipped"));
}

TEST_F(ReportFormatterTest, Pdf_ResultHtml)
{
    EXPECT_EQ(Formatter::resultToHtml(Result::Pass), "<b style=\"color:green;\">PASS</b>");
    EXPECT_EQ(Formatter::resultToHtml(Result::Fail), "<b style=\"color:red;\">FAIL</b>");
    EXPECT_EQ(Formatter::resultToHtml(Result::Incomplete), "<b style=\"color:darkorange;\">INCOMPLETE</b>");
    EXPECT_EQ(Formatter::resultToHtml(Result::Skipped), "<b>SKIPPED</b>");
    EXPECT_EQ(Formatter::resultToHtml(Result::Disabled), "<b>DISABLED</b>");
    EXPECT_EQ(Formatter::resultToHtml(std::nullopt), "<mark>Not provided</mark>");
}

// =============================================================================
// Report writers: nothing for an incomplete board, and no stale "last" report
// =============================================================================

class IncompleteWriterTest : public ReportFormatterTest {
protected:
    static constexpr auto s_title = "FrasyIncompleteWriterTest";

    // Where the writers put their "last" report, relative to the working directory.
    const fs::path m_json = fs::current_path() / "logs" / s_title / "json" / "last_uut1.json";
    const fs::path m_kvp  = fs::current_path() / ".." / "logs" / s_title / "smt" / "last_uut1.txt";
    const fs::path m_pdf  = fs::current_path() / ".." / "logs" / s_title / "client" / "last_uut1.pdf";

    void TearDown() override
    {
        std::error_code ec;
        fs::remove_all(fs::current_path() / "logs" / s_title, ec);
        fs::remove_all(fs::current_path() / ".." / "logs" / s_title, ec);
        ReportFormatterTest::TearDown();
    }

    static void stale(const fs::path& file)
    {
        fs::create_directories(file.parent_path());
        std::ofstream(file) << "previous board";
    }

    sol::table incomplete()
    {
        return table(std::string("{ info = { title = '") + s_title +
                     "', uut = 1, result = 'incomplete', pass = false }, ib = {}, sequences = {} }");
    }
};

TEST_F(IncompleteWriterTest, Json_NothingWrittenAndStaleReportRemoved)
{
    stale(m_json);
    EXPECT_TRUE(Frasy::Report::Json::makeReport(incomplete(), {"a.json"}).empty());
    EXPECT_FALSE(fs::exists(m_json));
    EXPECT_FALSE(fs::exists(m_json.parent_path() / "fail" / "a.json"));
}

TEST_F(IncompleteWriterTest, Kvp_NothingWrittenAndStaleReportRemoved)
{
    stale(m_kvp);
    EXPECT_TRUE(Frasy::Report::Kvp::makeReport(incomplete(), {"a.txt"}).empty());
    EXPECT_FALSE(fs::exists(m_kvp));
    EXPECT_FALSE(fs::exists(m_kvp.parent_path() / "fail" / "a.txt"));
}

TEST_F(IncompleteWriterTest, Pdf_NothingWrittenAndStaleReportRemoved)
{
    stale(m_pdf);
    EXPECT_TRUE(Frasy::Report::PDF::makeReport(incomplete(), {"a.pdf"}).empty());
    EXPECT_FALSE(fs::exists(m_pdf));
    EXPECT_FALSE(fs::exists(m_pdf.parent_path() / "fail" / "a.pdf"));
}
