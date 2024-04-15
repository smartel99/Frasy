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

#include <Brigerad.h>
#include <filesystem>
#include <json.hpp>
#include <map>
#include <vector>

namespace Frasy
{
class ResultViewer : public Brigerad::Layer
{
    using ExpectationDetails = std::map<std::string, nlohmann::json>;
    struct TestResult
    {
        std::string                     Name         = {};
        double                          Duration     = 0.0;
        bool                            Enabled      = true;
        bool                            Skipped      = true;
        bool                            Passed       = true;
        std::vector<ExpectationDetails> Expectations = {};
    };
    struct SequenceResult
    {
        std::string                       Name     = {};
        double                            Duration = 0.0;
        bool                              Enabled  = true;
        bool                              Skipped  = true;
        bool                              Passed   = true;
        std::map<std::string, TestResult> Tests    = {};
    };
    struct OverallTestResult
    {
        std::string                           SerialNumber = {};
        double                                Duration     = 0.0;
        std::string                           Date         = {};
        bool                                  Passed       = true;
        std::string                           Version      = {};
        int                                   Uut          = 0;
        std::map<std::string, SequenceResult> Sequences    = {};
    };
    struct LogInfo
    {
        std::string                     Name         = {};
        std::string                     Path         = {};
        std::filesystem::file_time_type LastModified = {};
        OverallTestResult               Results      = {};
        bool                            IsGood       = true;
    };

public:
    ResultViewer() noexcept;
    ~ResultViewer() override = default;

    void onImGuiRender() override;

    void setVisibility(bool visibility);

private:
    void        RenderLog(const OverallTestResult& log);
    void        RenderSequence(const SequenceResult& sequence);
    void        RenderTest(const TestResult& test);
    static void RenderExpectation(const ExpectationDetails& expectation);

    bool                                         AreLogsNew();
    static std::vector<LogInfo>                  LoadLogs(bool loadFiles);
    static OverallTestResult                     LoadResults(const std::string& path);
    static std::map<std::string, SequenceResult> LoadSequences(const nlohmann::json& sequences);
    static std::map<std::string, TestResult>     LoadTests(const nlohmann::json& tests);
    static std::vector<ExpectationDetails>       LoadExpectations(const nlohmann::json& expectations);

    static std::string MakeStringFromJson(const std::string& key, const nlohmann::json& value);


private:
    bool                 m_isVisible         = false;
    std::vector<LogInfo> m_logs              = {};
    bool                 m_isFirstPassOfLogs = true;

    static constexpr const char* s_windowName       = "Last Results";
    static constexpr const char* s_lastLogsPath     = "logs/last";
    static constexpr const char* s_logFileExtension = ".json";
};
}    // namespace Frasy
#endif    // FRASY_SRC_LAYERS_RESULT_VIEWER_H
