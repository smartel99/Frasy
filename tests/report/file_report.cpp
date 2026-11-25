#include "utils/lua/orchestrator/orchestrator.h"
#include "utils/UutState.h"

#include "log_once.h"
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

using Frasy::UutState;
using Orchestrator = Frasy::Lua::Orchestrator;
namespace fs       = std::filesystem;

// =============================================================================
// Orchestrator::fileReport — state of a board and where its report is archived
// =============================================================================

class FileReportTest : public ::testing::Test {
protected:
    fs::path m_root;
    fs::path m_report;

    static void SetUpTestSuite() { initLogOnce(); }

    void SetUp() override
    {
        const auto* test = ::testing::UnitTest::GetInstance()->current_test_info();
        m_root           = fs::temp_directory_path() / "frasy_tests" / test->test_suite_name() / test->name();
        fs::remove_all(m_root);
        for (const auto folder :
             {Orchestrator::passSubdirectory, Orchestrator::failSubdirectory, Orchestrator::debugSubdirectory}) {
            fs::create_directories(m_root / "archive" / folder);
        }
        fs::create_directories(m_root / "last");
        m_report = m_root / "last" / "1.json";
    }

    void TearDown() override
    {
        std::error_code ec;
        fs::remove_all(m_root, ec);
    }

    void writeReport(const std::string& content) const { std::ofstream(m_report) << content; }

    [[nodiscard]] std::size_t archived(std::string_view folder) const
    {
        const auto dir = m_root / "archive" / folder;
        return fs::exists(dir) ? std::distance(fs::directory_iterator(dir), fs::directory_iterator {}) : 0;
    }

    [[nodiscard]] std::size_t archivedAnywhere() const
    {
        return archived(Orchestrator::passSubdirectory) + archived(Orchestrator::failSubdirectory) +
               archived(Orchestrator::debugSubdirectory);
    }
};

TEST_F(FileReportTest, Pass_ArchivedInPass)
{
    writeReport(R"({ "info": { "format_version": 2.0, "result": "pass", "serial": "SN1" },
                     "sequences": { "S": { "result": "pass", "tests": { "T": { "result": "pass" } } } } })");
    EXPECT_EQ(Orchestrator::fileReport(m_report, m_root / "archive", 1), UutState::Passed);
    EXPECT_EQ(archived(Orchestrator::passSubdirectory), 1U);
    EXPECT_EQ(archivedAnywhere(), 1U);
    EXPECT_TRUE(fs::exists(m_report));
}

TEST_F(FileReportTest, Fail_ArchivedInFail)
{
    writeReport(R"({ "info": { "format_version": 2.0, "result": "fail", "serial": "SN1" },
                     "sequences": { "S": { "result": "fail", "tests": {} } } })");
    EXPECT_EQ(Orchestrator::fileReport(m_report, m_root / "archive", 1), UutState::Failed);
    EXPECT_EQ(archived(Orchestrator::failSubdirectory), 1U);
}

TEST_F(FileReportTest, IncompleteWithFailure_ArchivedInDebug)
{
    writeReport(R"({ "info": { "format_version": 2.0, "result": "incomplete", "serial": "SN1" },
                     "sequences": { "S": { "result": "incomplete", "tests": { "T1": { "result": "disabled" },
                                                                              "T2": { "result": "fail" } } } } })");
    EXPECT_EQ(Orchestrator::fileReport(m_report, m_root / "archive", 1), UutState::IncompleteFailed);
    EXPECT_EQ(archived(Orchestrator::debugSubdirectory), 1U);
    EXPECT_EQ(archivedAnywhere(), 1U);
}

TEST_F(FileReportTest, IncompleteWithoutFailure_ArchivedInDebug)
{
    writeReport(R"({ "info": { "format_version": 2.0, "result": "incomplete", "serial": "SN1" },
                     "sequences": { "S": { "result": "disabled", "tests": {} } } })");
    EXPECT_EQ(Orchestrator::fileReport(m_report, m_root / "archive", 1), UutState::IncompletePassed);
    EXPECT_EQ(archived(Orchestrator::debugSubdirectory), 1U);
}

TEST_F(FileReportTest, V1Report_StillFiled)
{
    writeReport(R"({ "info": { "pass": true, "serial": "SN1" },
                     "sequences": { "S": { "pass": true, "skipped": false, "enabled": true, "tests": {} } } })");
    EXPECT_EQ(Orchestrator::fileReport(m_report, m_root / "archive", 1), UutState::Passed);
    EXPECT_EQ(archived(Orchestrator::passSubdirectory), 1U);
}

TEST_F(FileReportTest, V1Incomplete_ArchivedInDebug)
{
    // A disabled test and a passing one: the sequence reads incomplete, not failed.
    writeReport(R"({ "info": { "pass": false, "serial": "SN1" },
                     "sequences": { "S": { "pass": false, "skipped": false, "enabled": true, "tests": {
                         "T1": { "pass": false, "skipped": true, "enabled": false },
                         "T2": { "pass": true, "skipped": false, "enabled": true } } } } })");
    EXPECT_EQ(Orchestrator::fileReport(m_report, m_root / "archive", 1), UutState::IncompletePassed);
    EXPECT_EQ(archived(Orchestrator::debugSubdirectory), 1U);
    EXPECT_EQ(archivedAnywhere(), 1U);
}

TEST_F(FileReportTest, V1IncompleteWithFailure_ArchivedInDebug)
{
    writeReport(R"({ "info": { "pass": false, "serial": "SN1" },
                     "sequences": { "S": { "pass": false, "skipped": false, "enabled": true, "tests": {
                         "T1": { "pass": false, "skipped": true, "enabled": false },
                         "T2": { "pass": false, "skipped": false, "enabled": true } } } } })");
    EXPECT_EQ(Orchestrator::fileReport(m_report, m_root / "archive", 1), UutState::IncompleteFailed);
    EXPECT_EQ(archived(Orchestrator::debugSubdirectory), 1U);
}

TEST_F(FileReportTest, InvalidResult_ErrorAndNotArchived)
{
    writeReport(R"({ "info": { "format_version": 2, "result": "great", "serial": "SN1" }, "sequences": {} })");
    EXPECT_EQ(Orchestrator::fileReport(m_report, m_root / "archive", 1), UutState::Error);
    EXPECT_EQ(archivedAnywhere(), 0U);
    EXPECT_TRUE(fs::exists(m_report));
}

TEST_F(FileReportTest, NewerVersion_ErrorAndNotArchived)
{
    writeReport(R"({ "info": { "format_version": 99, "result": "pass" }, "sequences": {} })");
    EXPECT_EQ(Orchestrator::fileReport(m_report, m_root / "archive", 1), UutState::Error);
    EXPECT_EQ(archivedAnywhere(), 0U);
}

TEST_F(FileReportTest, NotJson_ErrorAndNotArchived)
{
    writeReport("{ this is not json");
    EXPECT_EQ(Orchestrator::fileReport(m_report, m_root / "archive", 1), UutState::Error);
    EXPECT_EQ(archivedAnywhere(), 0U);
}

TEST_F(FileReportTest, NonStringSerial_ErrorAndNotArchived)
{
    writeReport(R"({ "info": { "format_version": 2, "result": "pass", "serial": 12 }, "sequences": {} })");
    EXPECT_EQ(Orchestrator::fileReport(m_report, m_root / "archive", 1), UutState::Error);
    EXPECT_EQ(archivedAnywhere(), 0U);
}

TEST_F(FileReportTest, ArchiveFails_StateKept)
{
    writeReport(R"({ "info": { "format_version": 2, "result": "fail", "serial": "SN1" },
                     "sequences": { "S": { "result": "fail", "tests": {} } } })");
    // No archive folders: the copy fails, the board's state stands.
    EXPECT_EQ(Orchestrator::fileReport(m_report, m_root / "missing", 1), UutState::Failed);
    EXPECT_TRUE(fs::exists(m_report));
}
