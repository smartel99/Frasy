/**
 * @file    impedance_scan.h
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

#ifndef FRASY_UTILS_IMPEDANCE_ANALYZER_IMPEDANCE_SCAN_H
#define FRASY_UTILS_IMPEDANCE_ANALYZER_IMPEDANCE_SCAN_H

#include <json.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace Frasy::ImpedanceAnalysis {

/// The serializer writes NaN as `null`. Missing or invalid numbers are represented by NaN in this module.
inline constexpr double s_nan = std::numeric_limits<double>::quiet_NaN();

/// Result of validating one field of DAQ_ImpedanceResults against the report.
enum class FieldStatus : uint8_t {
    Ok,           //!< Present with the expected type.
    Missing,      //!< Key is absent from the report.
    Null,         //!< Key is present but null. For measured values, this is how NaN is serialized.
    WrongType,    //!< Present, but with an unexpected JSON type.
};

struct FieldCheck {
    std::string name;
    std::string expectedType;
    FieldStatus status     = FieldStatus::Missing;
    bool        acceptable = false;    //!< False if the field makes the structure differ from DAQ_ImpedanceResults.
};

/// One entry of `measures`. NaN when the DAQ timed out (serialized as null).
struct Sample {
    double confidence = s_nan;
    double value      = s_nan;
    double vin        = s_nan;
    double vout       = s_nan;
    double timeMs     = s_nan;    //!< Since the first measure of the series was triggered. NaN in older reports.
};

/// Acceptance limits of the expectation the measurement belongs to.
struct Limits {
    std::optional<double> min      = std::nullopt;
    std::optional<double> max      = std::nullopt;
    std::optional<double> expected = std::nullopt;

    [[nodiscard]] bool hasBand() const { return min.has_value() && max.has_value(); }
};

/// How well the measurement can discriminate against the acceptance band.
enum class Relevance : uint8_t {
    Unknown,     //!< Not enough samples, or no band to compare against.
    Good,        //!< Measurement noise is a small part of the band.
    Marginal,    //!< Noise takes a large part of the band, results may flip close to the limits.
    Poor,        //!< Noise is as wide as the band: a single reading is not meaningful.
};

struct Stats {
    std::size_t count        = 0;    //!< Samples, including invalid ones.
    std::size_t invalidCount = 0;    //!< Samples that are NaN (timeouts).
    double      mean         = s_nan;
    double      stdDev       = s_nan;    //!< Sample standard deviation. NaN with fewer than 2 valid samples.
    double      min          = s_nan;
    double      max          = s_nan;
    double      first        = s_nan;
    double      last         = s_nan;
    double      peakToPeak   = s_nan;
    double      inBandRatio  = s_nan;    //!< Ratio of valid samples inside the band. NaN without a band.
    double      bandUsage    = s_nan;    //!< 6 * stdDev / band width. NaN without a band.
    Relevance   relevance    = Relevance::Unknown;
};

/// A DAQ_ImpedanceResults found in an expectation's `extra`.
struct ImpedanceMeasurement {
    std::string             path;        //!< Where it was found, relative to the expectation (e.g. "extra.result").
    nlohmann::json          parameters;    //!< Every field except `measures`.
    std::vector<FieldCheck> fields;        //!< Validation of each field of DAQ_ImpedanceResults.
    std::vector<Sample>     measures;
    Sample                  summary;       //!< Top level confidence, value, vin and vout.
    std::vector<std::string> issues;       //!< Human readable inconsistencies.

    /// True if every field of DAQ_ImpedanceResults is present with the right type.
    [[nodiscard]] bool structureOk() const;

    /// True if there is something to analyse: the structure matches and `multiple` produced more than one measure.
    [[nodiscard]] bool isAnalyzable() const;
};

/// The expectation holding the measurement.
struct ExpectationInfo {
    std::string           sequence;
    std::string           test;
    std::string           name;
    std::string           method;
    bool                  pass = true;
    std::optional<double> value = std::nullopt;    //!< The expectation's own value, if it is a number.
    Limits                limits;
};

struct MeasureReport {
    ExpectationInfo      expectation;
    ImpedanceMeasurement measurement;
};

struct ScanResult {
    std::size_t                expectationsScanned = 0;
    std::vector<MeasureReport> found;
};

/**
 * @brief Compute statistics over `values`, ignoring NaN.
 * @param values Samples to analyse.
 * @param limits Acceptance band of the expectation, used for the in-band ratio and the relevance.
 */
[[nodiscard]] Stats computeStats(std::span<const double> values, const Limits& limits);

/// Which value of a sample to look at.
enum class Channel : uint8_t { Value, Vin, Vout, Confidence };

[[nodiscard]] std::vector<double> extractChannel(std::span<const Sample> samples, Channel channel);

/// @return The time of every sample in ms, or nothing if one of them has no time (reports from before `time` existed).
[[nodiscard]] std::vector<double> extractTimes(std::span<const Sample> samples);

/// Fewest valid samples needed to tell if a signal is stable.
inline constexpr std::size_t s_minStabilitySamples = 6;

/// Tells whether, and from where, a signal stays around its final value.
struct Stability {
    bool        stable     = false;
    double      start      = s_nan;    //!< `x` of the first sample of the stable run. NaN if not stable.
    std::size_t firstIndex = 0;        //!< Index of that sample in the input.
    double      reference  = s_nan;    //!< Final value, the median of the last half of the samples.
    double      tolerance  = s_nan;    //!< The signal is stable when it stays within reference +/- tolerance.
};

/// Excitation voltage of the DAQ when the measurement does not set its `amplitude` (automatic), in V.
inline constexpr double s_defaultExcitationVoltage = 0.1;

/// How far vin + vout may be from the excitation voltage, relative to it.
inline constexpr double s_excitationToleranceBelow = 0.30;
inline constexpr double s_excitationToleranceAbove = 0.05;

/// Whether VEXC ~ VIN + VOUT. When an external signal disturbs the measurement, it does not hold.
struct ExcitationCheck {
    bool        ok        = false;    //!< True if every valid sample is within the limits. False without sample.
    std::size_t samples   = 0;        //!< Samples with a valid vin and vout.
    double      expected  = s_nan;    //!< The excitation voltage, VEXC.
    double      min       = s_nan;    //!< Lowest accepted vin + vout.
    double      max       = s_nan;    //!< Highest accepted vin + vout.
    double      mean      = s_nan;    //!< Mean of vin + vout.
    double      worst     = s_nan;    //!< vin + vout of the sample the furthest from the excitation voltage.
};

/**
 * @brief Check that VEXC ~ VIN + VOUT: the sum must be between VEXC - 30 % and VEXC + 5 %.
 * @details VEXC is the `amplitude` of the measurement, or `s_defaultExcitationVoltage` when it is automatic (0).
 *          Samples with a NaN vin or vout (timeouts) are ignored.
 */
[[nodiscard]] ExcitationCheck checkExcitation(const ImpedanceMeasurement& measurement);

/**
 * @brief Find after what delay a signal becomes stable.
 * @details The signal is stable if, from some sample on, it stays within a tolerance around its final value, and that
 *          holds for at least the last half of the samples.
 *          - The final value is the median of the last half.
 *          - The tolerance is 4 times the noise, estimated from the difference between consecutive samples of the last
 *            half (so a drift or a slope is not mistaken for noise), but never less than 0.1 % of the final value.
 *          NaN samples are ignored. At least 6 valid samples are needed.
 * @param values Samples to analyse.
 * @param x      Position of each sample, typically its time in ms. Same size as `values`.
 */
[[nodiscard]] Stability computeStability(std::span<const double> values, std::span<const double> x);

/**
 * @brief Tells if a JSON value looks like a DAQ_ImpedanceResults.
 * @details This is intentionally tolerant: it is true when `measures` is present, or when at least 3 of the fields
 *          specific to the impedance results are. Missing and mistyped fields are reported by `scanReport`.
 */
[[nodiscard]] bool looksLikeImpedanceResults(const nlohmann::json& json);

/**
 * @brief Crawl a report (`logs/last/<uut>.json`) and analyse every impedance result found in the `extra` of its
 *        expectations.
 * @param report Parsed report.
 * @return Measurements found, with the number of expectations that were looked at.
 */
[[nodiscard]] ScanResult scanReport(const nlohmann::json& report);

}    // namespace Frasy::ImpedanceAnalysis

#endif    // FRASY_UTILS_IMPEDANCE_ANALYZER_IMPEDANCE_SCAN_H
