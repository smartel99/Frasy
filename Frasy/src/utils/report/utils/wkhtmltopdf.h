/**
 * @file    wkhtmltopdf.h
 * @author  Sam Martel
 * @date    2026-02-03
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
 * not, see <https://www.gnu.org/licenses/>.
 */


#ifndef FRASY_SRC_UTILS_REPORT_FORMATTERS_UTILS_WKHTMLTOPDF_H
#define FRASY_SRC_UTILS_REPORT_FORMATTERS_UTILS_WKHTMLTOPDF_H
#include "wkhtmltox/pdf.h"

#include <Brigerad/Core/Log.h>

#include <vector>

namespace Frasy::Report::Formatter::Details {
class Wkhtmltopdf {
public:
    Wkhtmltopdf(std::string_view outputFilePath, std::string_view documentTitle)
    {
        Brigerad::Log::GetLogger("WKHTMLTOPDF")->set_level(spdlog::level::trace);
        if (!wkhtmltopdf_init(0)) { throw std::runtime_error("wkhtmltopdf_init failed"); }
        auto* globalSettings = wkhtmltopdf_create_global_settings();
        wkhtmltopdf_set_global_setting(globalSettings, "out", outputFilePath.data());
        wkhtmltopdf_set_global_setting(globalSettings, "documentTitle", documentTitle.data());
        m_converter = wkhtmltopdf_create_converter(globalSettings);
        if (m_converter == nullptr) {
            wkhtmltopdf_destroy_global_settings(globalSettings);
            throw std::runtime_error("wkhtmltopdf_create_converter failed");
        }
        wkhtmltopdf_set_error_callback(m_converter, errorCallback);
        wkhtmltopdf_set_warning_callback(m_converter, warningCallback);
        wkhtmltopdf_set_phase_changed_callback(m_converter, phaseChangedCallback);
        wkhtmltopdf_set_progress_changed_callback(m_converter, progressChangedCallback);
        wkhtmltopdf_set_finished_callback(m_converter, finishedCallback);
    }
    ~Wkhtmltopdf()
    {
        for (auto* obj : m_objects) {
            wkhtmltopdf_destroy_object_settings(obj);
        }
        wkhtmltopdf_destroy_converter(m_converter);
        wkhtmltopdf_deinit();
    }

    bool addContent(std::string_view html)
    {
        auto* objSettings = wkhtmltopdf_create_object_settings();
        if (objSettings) {
            m_objects.emplace_back(objSettings);
            wkhtmltopdf_add_object(m_converter, objSettings, html.data());
            return true;
        }
        return false;
    }

    bool convert() { return wkhtmltopdf_convert(m_converter) == 1; }

private:
    static void errorCallback([[maybe_unused]] wkhtmltopdf_converter* converter, const char* str)
    {
        BR_LOG_ERROR("WKHTMLTOPDF", "Error reported by wkhtmltopdf: {}", str);
    }

    static void warningCallback([[maybe_unused]] wkhtmltopdf_converter* converter, const char* str)
    {
        BR_LOG_WARN("WKHTMLTOPDF", "Warning reported by wkhtmltopdf: {}", str);
    }

    static void phaseChangedCallback(wkhtmltopdf_converter* converter)
    {
        int phase = wkhtmltopdf_current_phase(converter);
        BR_LOG_DEBUG("WKHTMLTOPDF",
                     "Phase changed to {}/{}: {}",
                     phase,
                     wkhtmltopdf_phase_count(converter),
                     wkhtmltopdf_phase_description(converter, phase));
    }
    static void progressChangedCallback([[maybe_unused]] wkhtmltopdf_converter* converter, int progress)
    {
        BR_LOG_DEBUG("WKHTMLTOPDF", "Progress changed to {}%", progress);
    }
    static void finishedCallback([[maybe_unused]] wkhtmltopdf_converter* converter, int success)
    {
        BR_LOG_DEBUG("WKHTMLTOPDF", "Conversion finished with code {}", success);
    }

private:
    wkhtmltopdf_converter*                    m_converter = nullptr;
    std::vector<wkhtmltopdf_object_settings*> m_objects;
};
}    // namespace Frasy::Report::Formatter::Details
#endif    // FRASY_SRC_UTILS_REPORT_FORMATTERS_UTILS_WKHTMLTOPDF_H
