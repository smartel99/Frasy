/**
 * @file    test.cpp
 * @brief   Unit tests for the impedance result scanner used by the impedance analyzer.
 */
#include <gtest/gtest.h>
#include <utils/impedance_analyzer/impedance_scan.h>

#include <algorithm>
#include <cmath>

using nlohmann::json;
using namespace Frasy::ImpedanceAnalysis;

namespace {

json makeSample(double value, double vin = 1.0, double vout = 0.5)
{
    return {{"confidence", 100.0}, {"value", value}, {"vin", vin}, {"vout", vout}};
}

/// Same layout as the report writer produces for DAQ_ImpedanceResults.
json makeImpedanceResults(const std::vector<double>& values)
{
    json measures = json::array();
    for (const double value : values) {
        measures.push_back(makeSample(value));
    }
    return {
      {"mode", 1.0},
      {"shape", 0.0},
      {"rangeResistor", 2.0},
      {"frequency", 1000.0},
      {"amplitude", 1.0},
      {"delay", 0.0},
      {"samplesToTake", 8.0},
      {"expectedValue", 100.0},
      {"favorSpeed", false},
      {"confidence", 100.0},
      {"value", values.empty() ? 0.0 : values.back()},
      {"vin", 1.0},
      {"vout", 0.5},
      {"measures", measures},
    };
}

json makeReport(const json& expectation)
{
    json report;
    report["sequences"]["seq"]["tests"]["test"]["expectations"] = json::array();
    report["sequences"]["seq"]["tests"]["test"]["expectations"].push_back(expectation);
    return report;
}

json makeExpectation(const json& extra, double value = 100.0)
{
    return {{"name", "R1"},
            {"method", "ToBeInRange"},
            {"pass", true},
            {"value", value},
            {"min", 90.0},
            {"max", 110.0},
            {"extra", extra}};
}

}    // namespace

// --- Discovery ---

TEST(ImpedanceScan, FindsResultDirectlyInExtra)
{
    const auto scan = scanReport(makeReport(makeExpectation(makeImpedanceResults({99.0, 100.0, 101.0}))));

    ASSERT_EQ(scan.found.size(), 1u);
    EXPECT_EQ(scan.expectationsScanned, 1u);
    EXPECT_EQ(scan.found[0].measurement.path, "extra");
    EXPECT_EQ(scan.found[0].measurement.measures.size(), 3u);
    EXPECT_EQ(scan.found[0].expectation.sequence, "seq");
    EXPECT_EQ(scan.found[0].expectation.test, "test");
    EXPECT_EQ(scan.found[0].expectation.limits.min, 90.0);
    EXPECT_EQ(scan.found[0].expectation.limits.max, 110.0);
}

TEST(ImpedanceScan, FindsResultsNestedInExtra)
{
    const json extra = {{"note", "text"}, {"result", makeImpedanceResults({1.0, 2.0})}, {"others", json::array({makeImpedanceResults({3.0, 4.0})})}};
    const auto scan  = scanReport(makeReport(makeExpectation(extra)));

    ASSERT_EQ(scan.found.size(), 2u);
    std::vector<std::string> paths = {scan.found[0].measurement.path, scan.found[1].measurement.path};
    std::ranges::sort(paths);
    EXPECT_EQ(paths[0], "extra.others[0]");
    EXPECT_EQ(paths[1], "extra.result");
}

TEST(ImpedanceScan, IgnoresExpectationsWithoutImpedanceResults)
{
    const auto noExtra = scanReport(makeReport({{"name", "x"}, {"pass", true}}));
    EXPECT_TRUE(noExtra.found.empty());
    EXPECT_EQ(noExtra.expectationsScanned, 1u);

    const auto unrelated = scanReport(makeReport(makeExpectation({{"reason", "text"}, {"value", 3.0}})));
    EXPECT_TRUE(unrelated.found.empty());
}

TEST(ImpedanceScan, ToleratesMalformedReport)
{
    EXPECT_TRUE(scanReport(json::object()).found.empty());
    EXPECT_TRUE(scanReport(json::array()).found.empty());
    // An empty `expectations` table is serialized as null.
    json report;
    report["sequences"]["s"]["tests"]["t"]["expectations"] = nullptr;
    EXPECT_TRUE(scanReport(report).found.empty());
}

// --- Structure ---

TEST(ImpedanceScan, ValidStructureHasNoIssue)
{
    const auto scan = scanReport(makeReport(makeExpectation(makeImpedanceResults({99.0, 100.0}))));
    ASSERT_EQ(scan.found.size(), 1u);
    EXPECT_TRUE(scan.found[0].measurement.structureOk());
    EXPECT_TRUE(scan.found[0].measurement.issues.empty());
    EXPECT_FALSE(scan.found[0].measurement.parameters.contains("measures"));
    EXPECT_TRUE(scan.found[0].measurement.parameters.contains("frequency"));
}

TEST(ImpedanceScan, ReportsMissingAndWrongTypeFields)
{
    auto results = makeImpedanceResults({99.0, 100.0});
    results.erase("frequency");
    results["favorSpeed"] = "no";

    const auto scan = scanReport(makeReport(makeExpectation(results)));
    ASSERT_EQ(scan.found.size(), 1u);
    const auto& measurement = scan.found[0].measurement;
    EXPECT_FALSE(measurement.structureOk());
    EXPECT_EQ(measurement.issues.size(), 2u);

    const auto status = [&measurement](std::string_view name) {
        return std::ranges::find(measurement.fields, name, &FieldCheck::name)->status;
    };
    EXPECT_EQ(status("frequency"), FieldStatus::Missing);
    EXPECT_EQ(status("favorSpeed"), FieldStatus::WrongType);
}

TEST(ImpedanceScan, NullMeasuresIsAStructureIssue)
{
    // An empty Lua table is serialized as null.
    auto results        = makeImpedanceResults({});
    results["measures"] = nullptr;

    const auto scan = scanReport(makeReport(makeExpectation(results)));
    ASSERT_EQ(scan.found.size(), 1u);
    EXPECT_FALSE(scan.found[0].measurement.structureOk());
    EXPECT_TRUE(scan.found[0].measurement.measures.empty());
}

TEST(ImpedanceScan, NullMeasuredValueIsATimeoutNotAStructureIssue)
{
    auto results        = makeImpedanceResults({99.0, 100.0, 101.0});
    results["measures"][1]["value"] = nullptr;

    const auto scan = scanReport(makeReport(makeExpectation(results, 101.0)));
    ASSERT_EQ(scan.found.size(), 1u);
    const auto& measurement = scan.found[0].measurement;
    EXPECT_TRUE(measurement.structureOk());
    EXPECT_TRUE(std::isnan(measurement.measures[1].value));
    EXPECT_EQ(measurement.issues.size(), 1u);    // "1 of 3 measures timed out"
}

TEST(ImpedanceScan, SingleMeasureIsFlagged)
{
    const auto scan = scanReport(makeReport(makeExpectation(makeImpedanceResults({100.0}))));
    ASSERT_EQ(scan.found.size(), 1u);
    EXPECT_EQ(scan.found[0].measurement.issues.size(), 1u);
}

TEST(ImpedanceScan, FlagsTopLevelValueNotMatchingLastMeasure)
{
    auto results     = makeImpedanceResults({99.0, 100.0, 101.0});
    results["value"] = 50.0;

    const auto scan = scanReport(makeReport(makeExpectation(results, 50.0)));
    ASSERT_EQ(scan.found.size(), 1u);
    EXPECT_EQ(scan.found[0].measurement.issues.size(), 1u);
}

TEST(ImpedanceScan, FlagsExpectationValueNotComingFromTheResult)
{
    const auto scan = scanReport(makeReport(makeExpectation(makeImpedanceResults({99.0, 100.0, 101.0}), 12345.0)));
    ASSERT_EQ(scan.found.size(), 1u);
    EXPECT_EQ(scan.found[0].measurement.issues.size(), 1u);
}

TEST(ImpedanceScan, ExpectationValueMatchingAnySampleIsFine)
{
    const auto scan = scanReport(makeReport(makeExpectation(makeImpedanceResults({99.0, 100.0, 101.0}), 101.0)));
    ASSERT_EQ(scan.found.size(), 1u);
    EXPECT_TRUE(scan.found[0].measurement.issues.empty());
}

TEST(ImpedanceScan, AnalyzableNeedsValidStructureAndSeveralMeasures)
{
    const auto analyzable = [](const json& results) {
        const auto scan = scanReport(makeReport(makeExpectation(results)));
        return scan.found.size() == 1 && scan.found[0].measurement.isAnalyzable();
    };

    EXPECT_TRUE(analyzable(makeImpedanceResults({99.0, 100.0})));
    EXPECT_FALSE(analyzable(makeImpedanceResults({100.0})));

    auto missing = makeImpedanceResults({99.0, 100.0});
    missing.erase("frequency");
    EXPECT_FALSE(analyzable(missing));

    auto nullMeasures        = makeImpedanceResults({99.0, 100.0});
    nullMeasures["measures"] = nullptr;
    EXPECT_FALSE(analyzable(nullMeasures));
}

// --- Statistics ---

TEST(ImpedanceStats, ComputesBasicStatistics)
{
    const std::vector<double> values = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    const auto                stats  = computeStats(values, {});

    EXPECT_EQ(stats.count, 8u);
    EXPECT_EQ(stats.invalidCount, 0u);
    EXPECT_DOUBLE_EQ(stats.mean, 5.0);
    EXPECT_DOUBLE_EQ(stats.min, 2.0);
    EXPECT_DOUBLE_EQ(stats.max, 9.0);
    EXPECT_DOUBLE_EQ(stats.peakToPeak, 7.0);
    EXPECT_DOUBLE_EQ(stats.first, 2.0);
    EXPECT_DOUBLE_EQ(stats.last, 9.0);
    EXPECT_NEAR(stats.stdDev, 2.13809, 1e-5);    // sample standard deviation
    EXPECT_TRUE(std::isnan(stats.inBandRatio));
    EXPECT_EQ(stats.relevance, Relevance::Unknown);
}

TEST(ImpedanceStats, IgnoresNaNSamples)
{
    const std::vector<double> values = {1.0, s_nan, 3.0};
    const auto                stats  = computeStats(values, {});

    EXPECT_EQ(stats.count, 3u);
    EXPECT_EQ(stats.invalidCount, 1u);
    EXPECT_DOUBLE_EQ(stats.mean, 2.0);
}

TEST(ImpedanceStats, EmptyAndSingleSampleHaveNoSpread)
{
    EXPECT_TRUE(std::isnan(computeStats({}, {}).mean));

    const std::vector<double> single = {5.0};
    const auto                stats  = computeStats(single, {.min = 0.0, .max = 10.0});
    EXPECT_DOUBLE_EQ(stats.mean, 5.0);
    EXPECT_TRUE(std::isnan(stats.stdDev));
    EXPECT_EQ(stats.relevance, Relevance::Unknown);
}

TEST(ImpedanceStats, InBandRatioSupportsOneSidedLimits)
{
    const std::vector<double> values = {1.0, 2.0, 3.0, 4.0};
    EXPECT_DOUBLE_EQ(computeStats(values, {.min = 2.5}).inBandRatio, 0.5);
    EXPECT_DOUBLE_EQ(computeStats(values, {.max = 3.0}).inBandRatio, 0.75);
    EXPECT_DOUBLE_EQ(computeStats(values, {.min = 2.0, .max = 3.0}).inBandRatio, 0.5);
}

TEST(ImpedanceStats, RelevanceComparesNoiseToBandWidth)
{
    const Limits band = {.min = 90.0, .max = 110.0};    // width 20

    // stdDev ~0.5 => 6 * 0.5 / 20 = 0.15
    EXPECT_EQ(computeStats(std::vector<double> {99.5, 100.0, 100.5}, band).relevance, Relevance::Good);
    // stdDev 2 => 0.6
    EXPECT_EQ(computeStats(std::vector<double> {98.0, 100.0, 102.0}, band).relevance, Relevance::Marginal);
    // stdDev 5 => 1.5
    EXPECT_EQ(computeStats(std::vector<double> {95.0, 100.0, 105.0}, band).relevance, Relevance::Poor);
}

TEST(ImpedanceStats, ExtractChannelPicksRequestedField)
{
    const std::vector<Sample> samples = {{.confidence = 1.0, .value = 2.0, .vin = 3.0, .vout = 4.0}};
    EXPECT_EQ(extractChannel(samples, Channel::Confidence), std::vector<double> {1.0});
    EXPECT_EQ(extractChannel(samples, Channel::Value), std::vector<double> {2.0});
    EXPECT_EQ(extractChannel(samples, Channel::Vin), std::vector<double> {3.0});
    EXPECT_EQ(extractChannel(samples, Channel::Vout), std::vector<double> {4.0});
}

// --- Time ---

TEST(ImpedanceScan, ReadsTheTimeOfEachSample)
{
    auto results = makeImpedanceResults({99.0, 100.0});
    results["measures"][0]["time"] = 120.0;
    results["measures"][1]["time"] = 250.0;

    const auto scan = scanReport(makeReport(makeExpectation(results)));
    ASSERT_EQ(scan.found.size(), 1u);
    EXPECT_TRUE(scan.found[0].measurement.structureOk());    // `time` is optional
    EXPECT_EQ(extractTimes(scan.found[0].measurement.measures), (std::vector<double> {120.0, 250.0}));
}

TEST(ImpedanceScan, NoTimesWhenOneSampleHasNone)
{
    auto results = makeImpedanceResults({99.0, 100.0});
    results["measures"][0]["time"] = 120.0;

    const auto scan = scanReport(makeReport(makeExpectation(results)));
    ASSERT_EQ(scan.found.size(), 1u);
    EXPECT_TRUE(extractTimes(scan.found[0].measurement.measures).empty());
}

// --- Stability ---

namespace {

std::vector<double> makeTimes(std::size_t count, double periodMs)
{
    std::vector<double> times;
    for (std::size_t i = 0; i < count; ++i) {
        times.push_back(static_cast<double>(i) * periodMs);
    }
    return times;
}

}    // namespace

TEST(ImpedanceStability, SettlingSignalIsStableAfterItsTransient)
{
    // Exponential settling with a time constant of 5 samples, plus a small alternating noise.
    std::vector<double> values;
    for (int i = 0; i < 200; ++i) {
        values.push_back(100.0 * (1.0 - std::exp(-i / 5.0)) + ((i % 2 == 0) ? 0.01 : -0.01));
    }
    const auto times     = makeTimes(values.size(), 10.0);
    const auto stability = computeStability(values, times);

    ASSERT_TRUE(stability.stable);
    // 100 * exp(-i / 5) is below the 0.1 tolerance from i = 35.
    EXPECT_GE(stability.firstIndex, 30u);
    EXPECT_LE(stability.firstIndex, 40u);
    EXPECT_DOUBLE_EQ(stability.start, static_cast<double>(stability.firstIndex) * 10.0);
    EXPECT_NEAR(stability.reference, 100.0, 0.1);
}

TEST(ImpedanceStability, FlatSignalIsStableFromTheStart)
{
    const std::vector<double> values(20, 42.0);
    const auto                stability = computeStability(values, makeTimes(values.size(), 5.0));
    ASSERT_TRUE(stability.stable);
    EXPECT_EQ(stability.firstIndex, 0u);
    EXPECT_DOUBLE_EQ(stability.start, 0.0);
}

TEST(ImpedanceStability, NoisySignalAroundAConstantIsStable)
{
    std::vector<double> values;
    for (int i = 0; i < 100; ++i) {
        values.push_back(100.0 + ((i % 3) - 1) * 0.5);
    }
    EXPECT_TRUE(computeStability(values, makeTimes(values.size(), 5.0)).stable);
}

TEST(ImpedanceStability, RampIsNotStable)
{
    std::vector<double> values;
    for (int i = 0; i < 100; ++i) {
        values.push_back(100.0 + i * 0.5);
    }
    EXPECT_FALSE(computeStability(values, makeTimes(values.size(), 5.0)).stable);
}

TEST(ImpedanceStability, SignalStillMovingAtTheEndIsNotStable)
{
    std::vector<double> values(50, 100.0);
    values.push_back(130.0);    // the last sample jumps
    EXPECT_FALSE(computeStability(values, makeTimes(values.size(), 5.0)).stable);
}

TEST(ImpedanceStability, TooFewSamplesAreNotStable)
{
    const std::vector<double> values(5, 100.0);
    EXPECT_FALSE(computeStability(values, makeTimes(values.size(), 5.0)).stable);
}

TEST(ImpedanceStability, IgnoresNaNSamplesAndKeepsTheirPositionOut)
{
    // Step from 0 to 100 at sample 4 with a timed out (NaN) sample in the middle of the settled part.
    std::vector<double> values = {0.0, 0.0, 0.0, 0.0, 100.0, 100.0, s_nan, 100.0, 100.0, 100.0, 100.0, 100.0};
    const auto          times  = makeTimes(values.size(), 10.0);

    const auto stability = computeStability(values, times);
    ASSERT_TRUE(stability.stable);
    EXPECT_EQ(stability.firstIndex, 4u);
    EXPECT_DOUBLE_EQ(stability.start, 40.0);
}

// --- Excitation ---

namespace {

ImpedanceMeasurement makeMeasurement(const std::vector<std::pair<double, double>>& vinVout, double amplitude = 0.0)
{
    ImpedanceMeasurement measurement;
    measurement.parameters = {{"amplitude", amplitude}};
    for (const auto& [vin, vout] : vinVout) {
        measurement.measures.push_back({.confidence = 100.0, .value = 1.0, .vin = vin, .vout = vout});
    }
    return measurement;
}

}    // namespace

TEST(ImpedanceExcitation, DefaultExcitationIs100mV)
{
    const auto check = checkExcitation(makeMeasurement({{0.098, 0.0}}));
    EXPECT_DOUBLE_EQ(check.expected, 0.1);
    EXPECT_DOUBLE_EQ(check.min, 0.07);    // -30 %
    EXPECT_DOUBLE_EQ(check.max, 0.105);   // +5 %
}

TEST(ImpedanceExcitation, SumNearTheExcitationVoltageIsOk)
{
    const auto check = checkExcitation(makeMeasurement({{0.098, 0.0}, {0.09, 0.012}, {0.1, 0.0}}));
    EXPECT_TRUE(check.ok);
    EXPECT_EQ(check.samples, 3u);
    EXPECT_NEAR(check.mean, 0.1, 1e-3);
}

TEST(ImpedanceExcitation, LimitsAreAsymmetric)
{
    // 30 % below is accepted, 5 % above too, but not much more.
    EXPECT_TRUE(checkExcitation(makeMeasurement({{0.071, 0.0}})).ok);
    EXPECT_FALSE(checkExcitation(makeMeasurement({{0.069, 0.0}})).ok);
    EXPECT_TRUE(checkExcitation(makeMeasurement({{0.104, 0.0}})).ok);
    EXPECT_FALSE(checkExcitation(makeMeasurement({{0.106, 0.0}})).ok);
}

TEST(ImpedanceExcitation, ExternalSignalIsAnError)
{
    // A live rail disturbs the measurement: vin + vout is far from the excitation voltage.
    const auto check = checkExcitation(makeMeasurement({{-0.4807, -0.9532}, {-0.4810, -0.9530}}));
    EXPECT_FALSE(check.ok);
    EXPECT_NEAR(check.worst, -1.4342, 1e-3);
}

TEST(ImpedanceExcitation, OneDisturbedSampleIsAnError)
{
    const auto check = checkExcitation(makeMeasurement({{0.098, 0.0}, {0.5, 0.2}, {0.098, 0.0}}));
    EXPECT_FALSE(check.ok);
    EXPECT_DOUBLE_EQ(check.worst, 0.7);
}

TEST(ImpedanceExcitation, UsesTheAmplitudeWhenSet)
{
    const auto check = checkExcitation(makeMeasurement({{0.9, 0.1}, {0.95, 0.05}}, 1.0));
    EXPECT_TRUE(check.ok);
    EXPECT_DOUBLE_EQ(check.expected, 1.0);
    EXPECT_DOUBLE_EQ(check.min, 0.7);
    EXPECT_DOUBLE_EQ(check.max, 1.05);
}

TEST(ImpedanceExcitation, IgnoresTimedOutSamplesButNeedsOne)
{
    EXPECT_TRUE(checkExcitation(makeMeasurement({{0.098, 0.0}, {s_nan, s_nan}})).ok);

    const auto none = checkExcitation(makeMeasurement({{s_nan, 0.0}, {0.0, s_nan}}));
    EXPECT_FALSE(none.ok);
    EXPECT_EQ(none.samples, 0u);
}
