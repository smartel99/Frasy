/**
 * @file    impedance_scan.cpp
 * @brief   Finds DAQ impedance results (DAQ_ImpedanceResults) in the expectations of a report and analyses them.
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this
 * program. If not, see <https://www.gnu.org/licenses/>.
 */

#include "impedance_scan.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <functional>
#include <iterator>
#include <numbers>
#include <numeric>
#include <string_view>
#include <utility>

namespace Frasy::ImpedanceAnalysis {

namespace {

enum class FieldKind : uint8_t {
    Parameter,    //!< Number written by ImpedancesParameters.
    Flag,         //!< Boolean.
    Measured,     //!< Number, or null when the DAQ timed out (NaN).
    List,         //!< Array of samples.
};

struct FieldSpec {
    std::string_view name;
    FieldKind        kind;
    bool             identifying;    //!< Specific enough to tell that an object is an impedance result.
};

/// Mirrors DAQ_ImpedanceResults in daq.lua. `value` and `confidence` are too generic to be identifying.
constexpr auto s_fields = std::to_array<FieldSpec>({
  {"mode",          FieldKind::Parameter, true },
  {"shape",         FieldKind::Parameter, true },
  {"rangeResistor", FieldKind::Parameter, true },
  {"frequency",     FieldKind::Parameter, true },
  {"amplitude",     FieldKind::Parameter, true },
  {"delay",         FieldKind::Parameter, true },
  {"samplesToTake", FieldKind::Parameter, true },
  {"expectedValue", FieldKind::Parameter, true },
  {"favorSpeed",    FieldKind::Flag,      true },
  {"confidence",    FieldKind::Measured,  false},
  {"value",         FieldKind::Measured,  false},
  {"vin",           FieldKind::Measured,  true },
  {"vout",          FieldKind::Measured,  true },
  {"measures",      FieldKind::List,      true },
});

/// Fields of one entry of `measures` (DAQ_ImpedanceMeasure).
constexpr auto s_sampleFields = std::to_array<std::string_view>({"confidence", "value", "vin", "vout"});

constexpr std::size_t s_minIdentifyingFields  = 3;
constexpr std::size_t s_minAnalyzableMeasures = 2;
constexpr double      s_sigmaSpan            = 6.0;    // +/- 3 sigma
constexpr double      s_goodBandUsage        = 0.5;
constexpr double      s_marginalBandUsage    = 1.0;
constexpr double      s_relativeTolerance    = 1e-9;

// --- Stability ---
constexpr double      s_noiseSigmas            = 4.0;
constexpr double      s_relativeToleranceFloor = 1e-3;    // 0.1 % of the final value
constexpr double      s_absoluteToleranceFloor = 1e-12;

double median(std::vector<double> values)
{
    std::ranges::sort(values);
    const auto middle = values.size() / 2;
    return values.size() % 2 == 1 ? values[middle] : (values[middle - 1] + values[middle]) / 2.0;
}

std::string_view typeName(FieldKind kind)
{
    switch (kind) {
        case FieldKind::Parameter:
            return "number";
        case FieldKind::Flag:
            return "boolean";
        case FieldKind::Measured:
            return "number or null";
        case FieldKind::List:
            return "array";
    }
    std::unreachable();
}

bool isOfKind(const nlohmann::json& json, FieldKind kind)
{
    switch (kind) {
        case FieldKind::Parameter:
            return json.is_number();
        case FieldKind::Flag:
            return json.is_boolean();
        case FieldKind::Measured:
            return json.is_number() || json.is_null();
        case FieldKind::List:
            return json.is_array();
    }
    std::unreachable();
}

double numberOrNan(const nlohmann::json& json)
{
    return json.is_number() ? json.get<double>() : s_nan;
}

bool nearlyEqual(double a, double b)
{
    if (std::isnan(a) || std::isnan(b)) { return std::isnan(a) && std::isnan(b); }
    return std::abs(a - b) <= s_relativeTolerance * std::max({1.0, std::abs(a), std::abs(b)});
}

FieldCheck checkField(const nlohmann::json& object, const FieldSpec& spec)
{
    FieldCheck check {.name = std::string {spec.name}, .expectedType = std::string {typeName(spec.kind)}};

    const auto it = object.find(spec.name);
    if (it == object.end()) { check.status = FieldStatus::Missing; }
    else if (it->is_null()) {
        // A null measured value is how NaN is serialized. A null list is how an empty table is serialized.
        check.status     = FieldStatus::Null;
        check.acceptable = spec.kind == FieldKind::Measured;
    }
    else if (isOfKind(*it, spec.kind)) {
        check.status     = FieldStatus::Ok;
        check.acceptable = true;
    }
    else {
        check.status = FieldStatus::WrongType;
    }
    return check;
}

std::string describe(const FieldCheck& check)
{
    switch (check.status) {
        case FieldStatus::Missing:
            return std::format("Field '{}' is missing.", check.name);
        case FieldStatus::Null:
            return std::format("Field '{}' is null, expected {}.", check.name, check.expectedType);
        case FieldStatus::WrongType:
            return std::format("Field '{}' has the wrong type, expected {}.", check.name, check.expectedType);
        case FieldStatus::Ok:
            break;
    }
    return {};
}

/// Returns false if the entry is not shaped like a DAQ_ImpedanceMeasure.
bool parseSample(const nlohmann::json& json, Sample& sample)
{
    if (!json.is_object()) { return false; }
    const bool complete = std::ranges::all_of(s_sampleFields, [&json](std::string_view key) {
        return json.contains(key) && (json.at(key).is_number() || json.at(key).is_null());
    });
    if (const auto it = json.find("confidence"); it != json.end()) { sample.confidence = numberOrNan(*it); }
    if (const auto it = json.find("value"); it != json.end()) { sample.value = numberOrNan(*it); }
    if (const auto it = json.find("vin"); it != json.end()) { sample.vin = numberOrNan(*it); }
    if (const auto it = json.find("vout"); it != json.end()) { sample.vout = numberOrNan(*it); }
    // Optional: added after the first reports were written.
    if (const auto it = json.find("time"); it != json.end()) { sample.timeMs = numberOrNan(*it); }
    return complete;
}

bool matchesAnyMeasuredValue(double value, const ImpedanceMeasurement& measurement)
{
    const auto matches = [value](const Sample& sample) {
        return nearlyEqual(value, sample.value) || nearlyEqual(value, sample.vin) || nearlyEqual(value, sample.vout);
    };
    return matches(measurement.summary) || std::ranges::any_of(measurement.measures, matches);
}

ImpedanceMeasurement analyse(std::string path, const nlohmann::json& object, const ExpectationInfo& expectation)
{
    ImpedanceMeasurement measurement;
    measurement.path       = std::move(path);
    measurement.parameters = nlohmann::json::object();

    // --- Structure ---
    for (const auto& spec : s_fields) {
        auto check = checkField(object, spec);
        if (!check.acceptable) { measurement.issues.push_back(describe(check)); }
        measurement.fields.push_back(std::move(check));
    }
    for (const auto& [key, value] : object.items()) {
        if (key != "measures") { measurement.parameters[key] = value; }
    }

    // --- Samples ---
    measurement.summary.confidence = numberOrNan(object.value("confidence", nlohmann::json {}));
    measurement.summary.value      = numberOrNan(object.value("value", nlohmann::json {}));
    measurement.summary.vin        = numberOrNan(object.value("vin", nlohmann::json {}));
    measurement.summary.vout       = numberOrNan(object.value("vout", nlohmann::json {}));

    std::size_t malformedSamples = 0;
    if (const auto it = object.find("measures"); it != object.end() && it->is_array()) {
        measurement.measures.reserve(it->size());
        for (const auto& entry : *it) {
            Sample sample;
            if (!parseSample(entry, sample)) { ++malformedSamples; }
            measurement.measures.push_back(sample);
        }
    }
    if (malformedSamples != 0) {
        measurement.issues.push_back(
          std::format("{} of {} entries of 'measures' are not a DAQ_ImpedanceMeasure (confidence, value, vin, vout).",
                      malformedSamples,
                      measurement.measures.size()));
    }

    // --- Consistency ---
    if (measurement.measures.size() == 1) {
        measurement.issues.push_back("Only one measure: `multiple` was not used, there is nothing to analyse.");
    }
    if (!measurement.measures.empty() && !nearlyEqual(measurement.summary.value, measurement.measures.back().value)) {
        measurement.issues.push_back("Top level value is not the value of the last measure.");
    }
    if (const auto timeouts = std::ranges::count_if(measurement.measures,
                                                    [](const Sample& sample) { return std::isnan(sample.value); });
        timeouts != 0) {
        measurement.issues.push_back(
          std::format("{} of {} measures timed out (null).", timeouts, measurement.measures.size()));
    }
    if (expectation.value.has_value() && !matchesAnyMeasuredValue(*expectation.value, measurement)) {
        measurement.issues.push_back(
          std::format("The expectation value ({}) is none of the measured value, vin or vout: it is derived from the "
                      "result, the graph shows the raw samples.",
                      *expectation.value));
    }

    return measurement;
}

ExpectationInfo makeExpectationInfo(std::string sequence,
                                    std::string test,
                                    const nlohmann::json& expectation)
{
    ExpectationInfo info {.sequence = std::move(sequence), .test = std::move(test)};

    const auto text = [&expectation](std::string_view key) -> std::string {
        const auto it = expectation.find(key);
        return (it != expectation.end() && it->is_string()) ? it->get<std::string>() : std::string {};
    };
    const auto number = [&expectation](std::string_view key) -> std::optional<double> {
        const auto it = expectation.find(key);
        if (it == expectation.end() || !it->is_number()) { return std::nullopt; }
        return it->get<double>();
    };

    info.name     = text("note");
    if (info.name.empty()) { info.name = text("name"); }
    info.method          = text("method");
    info.value           = number("value");
    info.limits.min      = number("min");
    info.limits.max      = number("max");
    info.limits.expected = number("expected");
    if (const auto it = expectation.find("pass"); it != expectation.end() && it->is_boolean()) {
        info.pass = it->get<bool>();
    }
    return info;
}

/// Calls `onFound` for every impedance result in `json`, without looking inside the ones found.
void findImpedanceResults(const nlohmann::json&                                           json,
                          const std::string&                                              path,
                          const std::function<void(const std::string&, const nlohmann::json&)>& onFound)
{
    if (looksLikeImpedanceResults(json)) {
        onFound(path, json);
    }
    else if (json.is_object()) {
        for (const auto& [key, value] : json.items()) {
            findImpedanceResults(value, std::format("{}.{}", path, key), onFound);
        }
    }
    else if (json.is_array()) {
        for (std::size_t i = 0; i < json.size(); ++i) {
            findImpedanceResults(json[i], std::format("{}[{}]", path, i), onFound);
        }
    }
}

}    // namespace

bool ImpedanceMeasurement::structureOk() const
{
    return std::ranges::all_of(fields, &FieldCheck::acceptable);
}

bool ImpedanceMeasurement::isAnalyzable() const
{
    return structureOk() && measures.size() >= s_minAnalyzableMeasures;
}

Stats computeStats(std::span<const double> values, const Limits& limits)
{
    Stats stats;
    stats.count = values.size();

    std::vector<double> valid;
    valid.reserve(values.size());
    std::ranges::copy_if(values, std::back_inserter(valid), [](double value) { return std::isfinite(value); });
    stats.invalidCount = stats.count - valid.size();
    if (valid.empty()) { return stats; }

    stats.first      = valid.front();
    stats.last       = valid.back();
    stats.mean       = std::accumulate(valid.begin(), valid.end(), 0.0) / static_cast<double>(valid.size());
    const auto [minIt, maxIt] = std::ranges::minmax_element(valid);
    stats.min        = *minIt;
    stats.max        = *maxIt;
    stats.peakToPeak = stats.max - stats.min;

    if (valid.size() > 1) {
        const double squares = std::accumulate(valid.begin(), valid.end(), 0.0, [&stats](double sum, double value) {
            return sum + (value - stats.mean) * (value - stats.mean);
        });
        stats.stdDev         = std::sqrt(squares / static_cast<double>(valid.size() - 1));
    }

    if (limits.min.has_value() || limits.max.has_value()) {
        const auto inBand = std::ranges::count_if(valid, [&limits](double value) {
            return (!limits.min.has_value() || value >= *limits.min) && (!limits.max.has_value() || value <= *limits.max);
        });
        stats.inBandRatio = static_cast<double>(inBand) / static_cast<double>(valid.size());
    }

    if (limits.hasBand() && !std::isnan(stats.stdDev)) {
        const double width = *limits.max - *limits.min;
        if (width > 0.0) {
            stats.bandUsage = s_sigmaSpan * stats.stdDev / width;
            if (stats.bandUsage <= s_goodBandUsage) { stats.relevance = Relevance::Good; }
            else if (stats.bandUsage <= s_marginalBandUsage) {
                stats.relevance = Relevance::Marginal;
            }
            else {
                stats.relevance = Relevance::Poor;
            }
        }
    }
    return stats;
}

std::vector<double> extractChannel(std::span<const Sample> samples, Channel channel)
{
    std::vector<double> values;
    values.reserve(samples.size());
    for (const auto& sample : samples) {
        switch (channel) {
            case Channel::Value:
                values.push_back(sample.value);
                break;
            case Channel::Vin:
                values.push_back(sample.vin);
                break;
            case Channel::Vout:
                values.push_back(sample.vout);
                break;
            case Channel::Confidence:
                values.push_back(sample.confidence);
                break;
        }
    }
    return values;
}

ExcitationCheck checkExcitation(const ImpedanceMeasurement& measurement)
{
    ExcitationCheck check;

    double amplitude = 0.0;
    if (const auto it = measurement.parameters.find("amplitude"); it != measurement.parameters.end() && it->is_number()) {
        amplitude = it->get<double>();
    }
    check.expected = amplitude > 0.0 ? amplitude : s_defaultExcitationVoltage;
    check.min      = check.expected * (1.0 - s_excitationToleranceBelow);
    check.max      = check.expected * (1.0 + s_excitationToleranceAbove);

    double sum   = 0.0;
    double worst = s_nan;
    for (const auto& sample : measurement.measures) {
        if (!std::isfinite(sample.vin) || !std::isfinite(sample.vout)) { continue; }
        const double total = sample.vin + sample.vout;
        ++check.samples;
        sum += total;
        if (std::isnan(worst) || std::abs(total - check.expected) > std::abs(worst - check.expected)) { worst = total; }
    }
    if (check.samples == 0) { return check; }

    check.mean  = sum / static_cast<double>(check.samples);
    check.worst = worst;
    check.ok    = std::ranges::all_of(measurement.measures, [&check](const Sample& sample) {
        if (!std::isfinite(sample.vin) || !std::isfinite(sample.vout)) { return true; }
        const double total = sample.vin + sample.vout;
        return total >= check.min && total <= check.max;
    });
    return check;
}

std::vector<double> extractTimes(std::span<const Sample> samples)
{
    std::vector<double> times;
    times.reserve(samples.size());
    for (const auto& sample : samples) {
        if (std::isnan(sample.timeMs)) { return {}; }
        times.push_back(sample.timeMs);
    }
    return times;
}

Stability computeStability(std::span<const double> values, std::span<const double> x)
{
    Stability result;

    std::vector<double>      valid;
    std::vector<double>      positions;
    std::vector<std::size_t> indices;
    for (std::size_t i = 0; i < std::min(values.size(), x.size()); ++i) {
        if (std::isfinite(values[i]) && std::isfinite(x[i])) {
            valid.push_back(values[i]);
            positions.push_back(x[i]);
            indices.push_back(i);
        }
    }
    const std::size_t count = valid.size();
    if (count < s_minStabilitySamples) { return result; }

    const std::size_t tailStart = count - count / 2;
    result.reference            = median({valid.begin() + static_cast<std::ptrdiff_t>(tailStart), valid.end()});

    // The noise is estimated from consecutive differences: a drift gives constant differences, not a spread.
    std::vector<double> differences;
    for (std::size_t i = tailStart + 1; i < count; ++i) {
        differences.push_back(valid[i] - valid[i - 1]);
    }
    double noise = 0.0;
    if (differences.size() >= 2) {
        const double mean = std::accumulate(differences.begin(), differences.end(), 0.0) /
                            static_cast<double>(differences.size());
        const double squares = std::accumulate(differences.begin(), differences.end(), 0.0, [mean](double sum, double d) {
            return sum + (d - mean) * (d - mean);
        });
        noise = std::sqrt(squares / static_cast<double>(differences.size() - 1)) / std::numbers::sqrt2;
    }
    result.tolerance = std::max({s_noiseSigmas * noise, s_relativeToleranceFloor * std::abs(result.reference), s_absoluteToleranceFloor});

    // Walk back from the end while the signal stays around the final value.
    std::size_t first = count;
    while (first > 0 && std::abs(valid[first - 1] - result.reference) <= result.tolerance) {
        --first;
    }

    // The whole last half must be inside: otherwise the signal is still moving at the end.
    if (first <= tailStart) {
        result.stable     = true;
        result.start      = positions[first];
        result.firstIndex = indices[first];
    }
    return result;
}

bool looksLikeImpedanceResults(const nlohmann::json& json)
{
    if (!json.is_object()) { return false; }
    if (json.contains("measures")) { return true; }
    const auto identifying = std::ranges::count_if(
      s_fields, [&json](const FieldSpec& spec) { return spec.identifying && json.contains(spec.name); });
    return static_cast<std::size_t>(identifying) >= s_minIdentifyingFields;
}

ScanResult scanReport(const nlohmann::json& report)
{
    ScanResult result;

    const auto sequences = report.find("sequences");
    if (sequences == report.end() || !sequences->is_object()) { return result; }

    for (const auto& [sequenceName, sequence] : sequences->items()) {
        const auto tests = sequence.find("tests");
        if (tests == sequence.end() || !tests->is_object()) { continue; }

        for (const auto& [testName, test] : tests->items()) {
            // An empty list of expectations is serialized as null, not as an array.
            const auto expectations = test.find("expectations");
            if (expectations == test.end() || !expectations->is_array()) { continue; }

            for (const auto& expectation : *expectations) {
                if (!expectation.is_object()) { continue; }
                ++result.expectationsScanned;

                const auto extra = expectation.find("extra");
                if (extra == expectation.end()) { continue; }

                const auto info = makeExpectationInfo(sequenceName, testName, expectation);
                findImpedanceResults(*extra, "extra", [&](const std::string& path, const nlohmann::json& found) {
                    result.found.push_back({.expectation = info, .measurement = analyse(path, found, info)});
                });
            }
        }
    }
    return result;
}

}    // namespace Frasy::ImpedanceAnalysis
