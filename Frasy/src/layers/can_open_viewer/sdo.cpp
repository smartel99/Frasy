/**
 * @file    sdo.cpp
 * @author  Samuel Martel
 * @date    2024-05-08
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
#include "sdo.h"

#include <Brigerad/Debug/Instrumentor.h>

#include <format>

#include <imgui.h>
#include <utils/imgui/table.h>

namespace Frasy::CanOpenViewer {

Sdo::Sdo(uint8_t nodeId) : m_tabBarName(std::format("Node {} SDO tab bar", nodeId))
{
}

void Sdo::onImGuiRender(CanOpen::Node& node)
{
    BR_PROFILE_FUNCTION();
    if (ImGui::BeginTabBar(m_tabBarName.c_str())) {
        ImGui::PushID(m_tabBarName.c_str());
        if (ImGui::BeginTabItem("Upload")) {
            renderUploadTab(node);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Download")) {
            renderDownloadTab(node);
            ImGui::EndTabItem();
        }
        ImGui::PopID();
        ImGui::EndTabBar();
    }
}

void Sdo::renderUploadTab(CanOpen::Node& node)
{
    BR_PROFILE_FUNCTION();
    // Read the completed requests
    purgeCompletedUploadRequests();

    renderUploadRequestMaker(node);

    ImGui::Separator();
    renderActiveUploadRequests();

    ImGui::Separator();
    renderUploadRequestHistory();
}

void Sdo::purgeCompletedUploadRequests()
{
    for (auto&& request : m_uploadRequestQueue | std::views::filter([](const auto& r) {
                              return r.status() == CanOpen::SdoUploadRequestStatus::Complete;
                          })) {
        auto                      result = request.future.get();
        FulfilledSdoUploadRequest requestResult {
          .index     = request.index(),
          .subIndex  = request.subIndex(),
          .hasFailed = !result.has_value(),
          .abortCode = std::format("{}", request.abortCode()),
        };

        if (result.has_value()) { requestResult.result = fmt::format("{:02x}", fmt::join(result.value(), ", ")); }
        else {
            requestResult.result = fmt::format("Request Failed: {}", result.error());
        }
        m_uploadRequestHistory.push_back(requestResult);
    }
    std::erase_if(m_uploadRequestQueue,
                  [](const auto& r) { return r.status() == CanOpen::SdoUploadRequestStatus::Complete; });
}

void Sdo::renderUploadRequestMaker(CanOpen::Node& node)
{
    ImGui::SliderInt("Index", &m_uploadRequestIndex, 0, 65535, "0x%04x");
    ImGui::SliderInt("Sub Index", &m_uploadRequestSubIndex, 0, 255, "0x%02x");
    ImGui::SliderInt("Timeout", &m_uploadRequestTimeout, 0, 65535, "%d ms");
    ImGui::Checkbox("Is Block", &m_uploadRequestIsBlock);
    if (ImGui::Button("Send")) {
        m_uploadRequestQueue.push_back(node.sdoInterface().uploadData(static_cast<uint16_t>(m_uploadRequestIndex),
                                                                      static_cast<uint8_t>(m_uploadRequestSubIndex),
                                                                      static_cast<uint16_t>(m_uploadRequestTimeout),
                                                                      m_uploadRequestIsBlock));
    }
}

void Sdo::renderActiveUploadRequests()
{
    ImGui::Text("Active Requests");
    Widget::Table("active upload requests table", 6, ImVec2 {0.0f, ImGui::GetContentRegionAvail().y / 3})
      .ColumnHeader("Status")
      .ColumnHeader("Index")
      .ColumnHeader("Sub Index")
      .ColumnHeader("Indicated Size")
      .ColumnHeader("Transferred Size")
      .ColumnHeader("Cancel")
      .ScrollFreeze()
      .FinishHeader()
      .Content(m_uploadRequestQueue | std::views::reverse,
               [](Widget::Table& table, CanOpen::SdoUploadDataResult& request) {
                   if (request.status() == CanOpen::SdoUploadRequestStatus::OnGoing) {
                       ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, s_onGoingRequestColor);
                   }

                   table.CellContentTextWrapped(request.status());
                   table.CellContentTextWrapped("0x{:04x}", request.index());
                   table.CellContentTextWrapped("0x{:02x}", request.subIndex());
                   table.CellContentTextWrapped(request.sizeIndicated());
                   table.CellContentTextWrapped(request.sizeTransferred());
                   if (table.CellContent([] { return ImGui::Button("Cancel"); })) { request.cancel(); }
               });
}

void Sdo::renderUploadRequestHistory()
{
    ImGui::Text("History");
    ImGui::SameLine();
    if (ImGui::Button("Clear")) { m_uploadRequestHistory.clear(); }
    Widget::Table("upload request history", 4)
      .ColumnHeader("Index")
      .ColumnHeader("Sub Index")
      .ColumnHeader("Abort Code")
      .ColumnHeader("Result")
      .ScrollFreeze()
      .FinishHeader()
      .Content(m_uploadRequestHistory | std::views::reverse,
               [](Widget::Table& table, const FulfilledSdoUploadRequest& request) {
                   if (request.hasFailed) { ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, s_requestFailedColor); }

                   table.CellContentTextWrapped("0x{:04x}", request.index);
                   table.CellContentTextWrapped("0x{:02x}", request.subIndex);
                   table.CellContentTextWrapped(request.abortCode);
                   table.CellContentTextWrapped(request.result);
               });
}

void Sdo::renderDownloadTab(CanOpen::Node& node)
{
}
}    // namespace Frasy::CanOpenViewer
