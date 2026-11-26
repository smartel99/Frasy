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

/** v1 report of an incomplete board: T1 disabled, T2 passed or failed. */
json v1Incomplete(bool t2Passed)
{
    auto report = json::parse(R"({ "info": { "pass": false }, "sequences": { "A": {
        "pass": false, "skipped": false, "enabled": true, "tests": {
            "T1": { "pass": false, "skipped": true, "enabled": false },
            "T2": { "pass": true, "skipped": false, "enabled": true } } } } })");
    report["sequences"]["A"]["tests"]["T2"]["pass"] = t2Passed;
    return report;
}

TEST(ReportResult, V1_DisabledScope_BoardIncomplete)
{
    EXPECT_EQ(ResultReader(v1Incomplete(true)).getInfo(), Result::Incomplete);
    EXPECT_EQ(ResultReader(v1Incomplete(false)).getInfo(), Result::Incomplete);
    auto disabledSequence = json::parse(R"({ "info": { "pass": false }, "sequences": {
        "A": { "pass": true, "skipped": false, "enabled": true, "tests": {} },
        "B": { "pass": false, "skipped": true, "enabled": false, "tests": {} } } })");
    EXPECT_EQ(ResultReader(disabledSequence).getInfo(), Result::Incomplete);
}

TEST(ReportResult, V1_NothingDisabled_BoardFromPass)
{
    const auto report = json::parse(R"({ "info": { "pass": false }, "sequences": { "A": {
        "pass": false, "skipped": false, "enabled": true, "tests": {
            "T1": { "pass": false, "skipped": false, "enabled": true } } } } })");
    EXPECT_EQ(ResultReader(report).getInfo(), Result::Fail);
}

TEST(ReportResult, V1_SequenceWithDisabledTest_IncompleteUnlessATestFailed)
{
    const ResultReader reader(v1Report());
    EXPECT_EQ(reader.read(v1Incomplete(true)["sequences"]["A"]), Result::Incomplete);
    EXPECT_EQ(reader.read(v1Incomplete(false)["sequences"]["A"]), Result::Fail);
}

TEST(ReportResult, V1_PendingTest_LeftOutOfSequence)
{
    auto report                                     = v1Incomplete(false);
    report["sequences"]["A"]["tests"]["T2"]["time"] = {{"stop", 0.0}};
    EXPECT_EQ(ResultReader(v1Report()).read(report["sequences"]["A"]), Result::Incomplete);
}

TEST(ReportResult, V1_NestedTests_NotReadRecursively)
{
    const auto sequence = json::parse(R"({ "pass": false, "tests": {
        "T1": { "enabled": false },
        "T2": { "pass": false, "tests": { "X": { "enabled": false } } } } })");
    EXPECT_EQ(ResultReader(v1Report()).read(sequence), Result::Fail);
}

TEST(ReportResult, V1_IncompleteBoard_FailedScopeOnlyWhenATestFailed)
{
    EXPECT_FALSE(Frasy::Report::hasFailedScope(v1Incomplete(true)));
    EXPECT_TRUE(Frasy::Report::hasFailedScope(v1Incomplete(false)));
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

// =============================================================================
// hasFailedScope
// =============================================================================

TEST(ReportResult, HasFailedScope_V2)
{
    const auto failed = json::parse(R"({
        "info": { "format_version": 2, "result": "incomplete" },
        "sequences": { "S": { "result": "fail", "tests": {} }, "D": { "result": "disabled", "tests": {} } }
    })");
    EXPECT_TRUE(Frasy::Report::hasFailedScope(failed));

    const auto passed = json::parse(R"({
        "info": { "format_version": 2, "result": "incomplete" },
        "sequences": { "S": { "result": "incomplete", "tests": {} }, "K": { "result": "skipped", "tests": {} },
                       "P": { "result": "pass", "tests": {} } }
    })");
    EXPECT_FALSE(Frasy::Report::hasFailedScope(passed));
}

TEST(ReportResult, HasFailedScope_V1)
{
    const auto failed = json::parse(R"({
        "info": { "pass": false },
        "sequences": { "S": { "pass": false, "skipped": false, "enabled": true, "tests": {} } }
    })");
    EXPECT_TRUE(Frasy::Report::hasFailedScope(failed));

    // Skipped and disabled sequences also had `pass = false` in v1: they aren't failures.
    const auto notRun = json::parse(R"({
        "info": { "pass": false },
        "sequences": { "K": { "pass": false, "skipped": true, "enabled": true, "tests": {} },
                       "D": { "pass": false, "skipped": true, "enabled": false, "tests": {} } }
    })");
    EXPECT_FALSE(Frasy::Report::hasFailedScope(notRun));
}

TEST(ReportResult, HasFailedScope_NoSequences)
{
    EXPECT_FALSE(Frasy::Report::hasFailedScope(v2Report()));
    EXPECT_FALSE(Frasy::Report::hasFailedScope(v1Report()));
}

TEST(ReportResult, HasFailedScope_IgnoresEntriesWithoutResult)
{
    // A product's onReport may add its own entries next to the scopes.
    const auto v2 = json::parse(R"({
        "info": { "format_version": 2, "result": "incomplete" },
        "sequences": { "S": { "result": "pass", "tests": {} }, "Summary": { "yield": "90%" }, "count": 3 }
    })");
    EXPECT_FALSE(Frasy::Report::hasFailedScope(v2));

    const auto v1 = json::parse(R"({
        "info": { "pass": true },
        "sequences": { "S": { "pass": true, "tests": {} }, "Summary": { "yield": "90%" } }
    })");
    EXPECT_FALSE(Frasy::Report::hasFailedScope(v1));
}

TEST(ReportResult, HasFailedScope_ThrowsOnUnreadableScope)
{
    const auto report = json::parse(R"({
        "info": { "format_version": 2, "result": "pass" },
        "sequences": { "S": { "result": "nope", "tests": {} } }
    })");
    EXPECT_THROW((void)Frasy::Report::hasFailedScope(report), ReportFormatError);
}

TEST(ReportResult, HasFailedScope_FailedTestUnderNonFailedSequence)
{
    // A sequence cut short by an error in a later section keeps the result of its earlier sections.
    const auto report = json::parse(R"({
        "info": { "format_version": 2, "result": "incomplete" },
        "sequences": { "S": { "result": "incomplete", "tests": { "T1": { "result": "disabled" },
                                                                 "T2": { "result": "fail" } } } }
    })");
    EXPECT_TRUE(Frasy::Report::hasFailedScope(report));
}

TEST(ReportResult, HasFailedScope_ThrowsOnUnreadableReport)
{
    EXPECT_THROW((void)Frasy::Report::hasFailedScope(json::parse(R"({ "sequences": {} })")), ReportFormatError);
}

// =============================================================================
// countTests, getResult(UutState), ResultCounts::toJson
// =============================================================================

TEST(ReportCountTests, V2_CountsEachResultAndIgnoresEntriesWithoutResult)
{
    const auto report = json::parse(R"({ "info": { "format_version": 2, "result": "incomplete" }, "sequences": {
        "S1": { "result": "incomplete", "tests": { "A": { "result": "pass" }, "B": { "result": "disabled" },
                                                   "extra": { "note": "onReport" } } },
        "S2": { "result": "fail", "tests": { "C": { "result": "fail" }, "D": { "result": "skipped" } } },
        "extra": { "note": "onReport" } } })");
    const auto counts = Frasy::Report::countTests(report);
    EXPECT_EQ(counts.Counts, (std::array<size_t, 5> {1, 1, 1, 1, 0}));
}

TEST(ReportCountTests, V1_TestsOfDisabledSequenceAreDisabled)
{
    const auto report = json::parse(R"({ "info": { "pass": true }, "sequences": {
        "S": { "pass": false, "skipped": true, "enabled": false, "tests": { "T": { "pass": false, "skipped": false } } },
        "R": { "pass": true, "skipped": false, "enabled": true, "tests": { "U": { "pass": true } } } } })");
    const auto counts = Frasy::Report::countTests(report);
    EXPECT_EQ(counts.get(Result::Disabled), 1U);
    EXPECT_EQ(counts.get(Result::Pass), 1U);
    EXPECT_EQ(counts.getTotal(), 2U);
}

TEST(ReportCountTests, NoSequences_Zero)
{
    EXPECT_EQ(Frasy::Report::countTests(json::parse(R"({ "info": { "pass": true } })")).getTotal(), 0U);
}

TEST(ReportCountTests, UnknownResult_Throws)
{
    const auto report = json::parse(R"({ "info": { "format_version": 2, "result": "fail" }, "sequences": {
        "S": { "result": "fail", "tests": { "T": { "result": "great" } } } } })");
    EXPECT_THROW((void)Frasy::Report::countTests(report), ReportFormatError);
}

TEST(ReportUutResult, EachState)
{
    using Frasy::UutState;
    using Frasy::Report::getUutResult;
    EXPECT_EQ(getUutResult(UutState::Passed), Result::Pass);
    EXPECT_EQ(getUutResult(UutState::Failed), Result::Fail);
    EXPECT_EQ(getUutResult(UutState::Disabled), Result::Disabled);
    EXPECT_EQ(getUutResult(UutState::IncompletePassed), Result::Incomplete);
    EXPECT_EQ(getUutResult(UutState::IncompleteFailed), Result::Incomplete);
    for (const auto state : {UutState::Idle, UutState::Waiting, UutState::Running, UutState::Error}) {
        EXPECT_EQ(getUutResult(state), std::nullopt) << Frasy::toString(state);
    }
}

TEST(ReportResultCounts, ToJson_EveryResultKeyed)
{
    Frasy::Report::ResultCounts counts;
    counts.add(Result::Pass);
    counts.add(Result::Incomplete);
    counts.add(Result::Incomplete);
    EXPECT_EQ(counts.toJson(),
              json::parse(R"({ "pass": 1, "fail": 0, "skipped": 0, "disabled": 0, "incomplete": 2 })"));
}

TEST(ReportCountTests, NoInfo_Throws)
{
    EXPECT_THROW((void)Frasy::Report::countTests(json::parse(R"({ "sequences": {} })")), ReportFormatError);
}

TEST(ReportCountTests, SequenceWithoutTests_Skipped)
{
    const auto report = json::parse(R"({ "info": { "format_version": 2, "result": "pass" }, "sequences": {
        "A": { "result": "pass" }, "B": { "result": "pass", "tests": [ 1, 2 ] },
        "C": { "result": "pass", "tests": { "T": { "result": "pass" } } } } })");
    EXPECT_EQ(Frasy::Report::countTests(report).getTotal(), 1U);
}
