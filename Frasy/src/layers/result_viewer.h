/**
 * @file    result_viewer.h
 * @author  Samuel Martel
 * @date    2023-05-01
 * @brief
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. If
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */

#ifndef FRASY_SRC_LAYERS_RESULT_VIEWER_H
#define FRASY_SRC_LAYERS_RESULT_VIEWER_H
#include "utils/communication/serial/device_map.h"
#include "utils/communication/serial/enumerator.h"
#include "utils/report/result.h"

#include <Brigerad.h>
#include <filesystem>
#include <json.hpp>
#include <map>
#include <optional>
#include <vector>

namespace Frasy {
class ResultViewer : public Brigerad::Layer {
public:
    using ExpectationDetails = std::map<std::string, nlohmann::json>;
    struct ExpectationResult {
        std::optional<Report::Result> Result;    //!< Nothing for an entry without a result.
        ExpectationDetails            Details;
    };
    struct TestResult {
        std::string                    Name;
        double                         Duration = 0.0;
        Report::Result                 Result   = Report::Result::Fail;
        std::vector<ExpectationResult> Expectations;
    };
    struct SequenceResult {
        std::string                       Name;
        double                            Duration = 0.0;
        Report::Result                    Result   = Report::Result::Fail;
        std::map<std::string, TestResult> Tests;
    };
    struct OverallTestResult {
        std::string                           SerialNumber = "<N/A>";
        double                                Duration     = 0.0;
        std::string                           Date         = "<N/A>";
        Report::Result                        Result       = Report::Result::Fail;
        bool                                  HasFailure   = false;    //!< A sequence or test failed.
        std::string                           Version      = "<N/A>";
        int                                   Uut          = 0;
        std::map<std::string, SequenceResult> Sequences;
    };
    struct LogInfo {
        std::string                     Name;
        std::string                     Path;
        std::filesystem::file_time_type LastModified;
        OverallTestResult               Results = {};
        bool                            IsGood  = true;
    };

    ResultViewer() noexcept;
    ~ResultViewer() override = default;

    void onImGuiRender() override;

    void setVisibility(bool visibility);

    /**
     * @brief Load a saved report, whatever its layout version.
     * @throws Report::ReportFormatError or nlohmann::json::exception if the report can't be read.
     */
    static OverallTestResult LoadResults(const std::string& path);

    /** @return Whether a scope's node starts expanded: failed and incomplete ones do. */
    static bool isOpenByDefault(std::optional<Report::Result> result);

    /**
     * @brief Text colour of a report's tab.
     * @param hasFailure Whether a sequence or test failed, for an incomplete board.
     * @return Red for a failure, orange for an incomplete board without one, nothing to keep the default.
     */
    static std::optional<uint32_t> getTabColor(Report::Result result, bool hasFailure);

    static constexpr uint32_t s_failColor       = 0xFF0000FF;
    static constexpr uint32_t s_incompleteColor = 0xFF008CFF;

private:
    void        RenderLog(const OverallTestResult& log, std::size_t index);
    void        RenderSequence(const SequenceResult& sequence, std::size_t index);
    void        RenderTest(const TestResult& test, std::size_t index);
    static void RenderExpectation(const ExpectationDetails& expectation);

    bool                                         AreLogsNew();
    static std::vector<LogInfo>                  LoadLogs(bool loadFiles);
    static std::map<std::string, SequenceResult> LoadSequences(const Report::ResultReader& reader,
                                                               const nlohmann::json&       sequences);
    static std::map<std::string, TestResult>     LoadTests(const Report::ResultReader& reader,
                                                           const nlohmann::json&       tests,
                                                           Report::Result              sequenceResult);
    static std::vector<ExpectationResult>        LoadExpectations(const Report::ResultReader& reader,
                                                                  const nlohmann::json&       expectations);
    static nlohmann::json                        SanitizeExpectation(const nlohmann::json& expectation);

    static std::string MakeStringFromJson(const std::string& key, const nlohmann::json& value);


    bool                 m_isVisible = false;
    std::vector<LogInfo> m_logs;
    std::vector<bool>    m_isFirstPassOfLogs = {};

    static constexpr const char* s_windowName       = "Last Results";
    static constexpr const char* s_lastLogsPath     = "logs/last";
    static constexpr const char* s_logFileExtension = ".json";
};
}    // namespace Frasy
#endif    // FRASY_SRC_LAYERS_RESULT_VIEWER_H
