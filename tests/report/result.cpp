#include "utils/report/result.h"

#include <gtest/gtest.h>

#include <utility>

using Frasy::Report::ReportFormatError;
using Frasy::Report::Result;
using Frasy::Report::ResultReader;
using json = nlohmann::json;

namespace {

json v1Report()
{
    return json::parse(R"({ "info": { "pass": true } })");
}

json v2Report()
{
    return json::parse(R"({ "info": { "format_version": 2, "result": "pass" } })");
}

json withVersion(json version)
{
    auto report                      = v2Report();
    report["info"]["format_version"] = std::move(version);
    return report;
}

Result readV1(const char* node, std::optional<Result> parent = std::nullopt)
{
    return ResultReader(v1Report()).read(json::parse(node), parent);
}

Result readV2(const char* node, std::optional<Result> parent = std::nullopt)
{
    return ResultReader(v2Report()).read(json::parse(node), parent);
}

}    // namespace

// =============================================================================
// toString / resultFromString
// =============================================================================

TEST(ReportResult, StringRoundTrip)
{
    for (auto result : {Result::Pass, Result::Fail, Result::Skipped, Result::Disabled, Result::Incomplete}) {
        const auto parsed = Frasy::Report::resultFromString(Frasy::Report::toString(result));
        ASSERT_TRUE(parsed.has_value());
        EXPECT_EQ(*parsed, result);
    }
}

TEST(ReportResult, ToString_MatchesLuaValues)
{
    EXPECT_EQ(Frasy::Report::toString(Result::Pass), "pass");
    EXPECT_EQ(Frasy::Report::toString(Result::Fail), "fail");
    EXPECT_EQ(Frasy::Report::toString(Result::Skipped), "skipped");
    EXPECT_EQ(Frasy::Report::toString(Result::Disabled), "disabled");
    EXPECT_EQ(Frasy::Report::toString(Result::Incomplete), "incomplete");
}

TEST(ReportResult, ToString_OutOfRangeIsUnknown)
{
    EXPECT_EQ(Frasy::Report::toString(static_cast<Result>(42)), "unknown");
}

TEST(ReportResult, FromString_RejectsUnknown)
{
    EXPECT_FALSE(Frasy::Report::resultFromString("PASS").has_value());
    EXPECT_FALSE(Frasy::Report::resultFromString("passed").has_value());
    EXPECT_FALSE(Frasy::Report::resultFromString("unknown").has_value());
    EXPECT_FALSE(Frasy::Report::resultFromString("").has_value());
}

// =============================================================================
// getFormatVersion
// =============================================================================

TEST(ReportResult, FormatVersion_MissingIsV1)
{
    EXPECT_EQ(Frasy::Report::getFormatVersion(v1Report()), 1);
}

TEST(ReportResult, FormatVersion_ReadFromInfo)
{
    EXPECT_EQ(Frasy::Report::getFormatVersion(v2Report()), 2);
}

TEST(ReportResult, FormatVersion_AcceptsWholeFloat)
{
    // SaveAsJson writes every Lua number as a double.
    EXPECT_EQ(Frasy::Report::getFormatVersion(json::parse(R"({ "info": { "format_version": 2.0 } })")), 2);
}

TEST(ReportResult, FormatVersion_ThrowsWithoutInfo)
{
    EXPECT_THROW((void)Frasy::Report::getFormatVersion(json::parse(R"({ "sequences": {} })")), ReportFormatError);
    EXPECT_THROW((void)Frasy::Report::getFormatVersion(json::parse(R"({ "info": 3 })")), ReportFormatError);
}

TEST(ReportResult, FormatVersion_ThrowsOnInvalidValue)
{
    for (const char* version : {R"("2")",
                                "2.5",
                                "0",
                                "-1",
                                "0.0",
                                "null",
                                "true",
                                "4294967298",
                                "4294967297",
                                "18446744073709551615",
                                "1e300"}) {
        EXPECT_THROW((void)Frasy::Report::getFormatVersion(withVersion(json::parse(version))), ReportFormatError)
          << version;
    }
}

// =============================================================================
// ResultReader
// =============================================================================

TEST(ReportResult, Reader_KeepsVersionAndInfo)
{
    const ResultReader v1(v1Report());
    EXPECT_EQ(v1.getFormatVersion(), 1);
    EXPECT_EQ(v1.getInfo(), Result::Pass);
    const ResultReader v2(withVersion(2.0));
    EXPECT_EQ(v2.getFormatVersion(), 2);
    EXPECT_EQ(v2.getInfo(), Result::Pass);
}

TEST(ReportResult, Reader_ThrowsOnNewerVersion)
{
    EXPECT_THROW(ResultReader(withVersion(Frasy::Report::s_currentFormatVersion + 1)), ReportFormatError);
}

TEST(ReportResult, Reader_ThrowsOnUnreadableInfo)
{
    EXPECT_THROW(ResultReader(json::parse(R"({ "info": { "format_version": 2 } })")), ReportFormatError);
    EXPECT_THROW(ResultReader(json::parse(R"({ "info": { "format_version": 2, "result": "ok" } })")),
                 ReportFormatError);
}

TEST(ReportResult, V1_ReadsBooleans)
{
    EXPECT_EQ(readV1(R"({ "pass": true })"), Result::Pass);
    EXPECT_EQ(readV1(R"({ "pass": false })"), Result::Fail);
    EXPECT_EQ(readV1(R"({ "pass": true, "skipped": false, "enabled": true })"), Result::Pass);
    EXPECT_EQ(readV1(R"({ "pass": false, "skipped": true, "enabled": true })"), Result::Skipped);
}

TEST(ReportResult, V1_DisabledWinsOverSkipped)
{
    EXPECT_EQ(readV1(R"({ "pass": false, "skipped": true, "enabled": false })"), Result::Disabled);
}

TEST(ReportResult, V1_TestOfSkippedOrDisabledSequence_TakesSequenceResult)
{
    // v1 never touched these tests: they kept the defaults.
    const char* untouched = R"({ "pass": false, "skipped": false })";
    EXPECT_EQ(readV1(untouched, Result::Disabled), Result::Disabled);
    EXPECT_EQ(readV1(untouched, Result::Skipped), Result::Skipped);
    EXPECT_EQ(readV1(untouched, Result::Fail), Result::Fail);
    EXPECT_EQ(readV1(untouched, Result::Pass), Result::Fail);
    EXPECT_EQ(readV1(untouched), Result::Fail);
}

TEST(ReportResult, V1_ExpectationInvertedAlreadyApplied)
{
    // Lua writes `pass` with the inversion applied; the reader must not flip it again.
    EXPECT_EQ(readV1(R"({ "pass": true, "inverted": true })"), Result::Pass);
}

TEST(ReportResult, V1_MissingPassIsFail)
{
    EXPECT_EQ(readV1(R"({ "name": "hand built" })"), Result::Fail);
}

TEST(ReportResult, V1_ThrowsOnNonBooleanField)
{
    for (const char* node : {R"({ "pass": "true" })",
                             R"({ "pass": true, "enabled": "yes" })",
                             R"({ "pass": true, "enabled": null })",
                             R"({ "pass": true, "skipped": 0 })"}) {
        EXPECT_THROW((void)readV1(node), ReportFormatError) << node;
    }
}

TEST(ReportResult, V2_ReadsResult)
{
    EXPECT_EQ(readV2(R"({ "result": "pass" })"), Result::Pass);
    EXPECT_EQ(readV2(R"({ "result": "fail" })"), Result::Fail);
    EXPECT_EQ(readV2(R"({ "result": "skipped" })"), Result::Skipped);
    EXPECT_EQ(readV2(R"({ "result": "disabled" })"), Result::Disabled);
    EXPECT_EQ(readV2(R"({ "result": "incomplete" })"), Result::Incomplete);
}

TEST(ReportResult, V2_IgnoresDeprecatedBooleansAndParent)
{
    EXPECT_EQ(readV2(R"({ "result": "disabled", "pass": true, "enabled": true })"), Result::Disabled);
    EXPECT_EQ(readV2(R"({ "result": "pass" })", Result::Skipped), Result::Pass);
}

TEST(ReportResult, V2_ThrowsOnMissingOrUnknownResult)
{
    for (const char* node : {R"({ "pass": true })", R"({ "result": "passed" })", R"({ "result": true })"}) {
        EXPECT_THROW((void)readV2(node), ReportFormatError) << node;
    }
}

TEST(ReportResult, ReadsWholeV2Report)
{
    const auto         report = json::parse(R"({
        "info": { "format_version": 2.0, "result": "incomplete", "pass": false },
        "sequences": { "S": { "result": "incomplete", "tests": {
            "T1": { "result": "disabled", "expectations": [] },
            "T2": { "result": "pass", "expectations": [ { "result": "pass", "pass": true, "inverted": true } ] }
        } } }
    })");
    const ResultReader reader(report);
    EXPECT_EQ(reader.getInfo(), Result::Incomplete);
    const auto sequence = reader.read(report["sequences"]["S"]);
    EXPECT_EQ(sequence, Result::Incomplete);
    EXPECT_EQ(reader.read(report["sequences"]["S"]["tests"]["T1"], sequence), Result::Disabled);
    EXPECT_EQ(reader.read(report["sequences"]["S"]["tests"]["T2"]["expectations"][0]), Result::Pass);
}
