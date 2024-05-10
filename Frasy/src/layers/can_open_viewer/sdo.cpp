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

#include "utils/communication/can_open/to_string.h"
#include "utils/imgui/table.h"
#include "utils/imgui/types.h"
#include "utils/misc/visit.h"

#include <Brigerad/Debug/Instrumentor.h>


#include <format>

#include <imgui.h>

namespace Frasy::CanOpenViewer {


constexpr std::string_view toString(VarType type)
{
    switch (type) {
        case VarType::Boolean: return "bool";
        case VarType::Signed8: return "int8_t";
        case VarType::Signed16: return "int16_t";
        case VarType::Signed32: return "int32_t";
        case VarType::Signed64: return "int64_t";
        case VarType::Unsigned8: return "uint8_t";
        case VarType::Unsigned16: return "uint16_t";
        case VarType::Unsigned32: return "uint32_t";
        case VarType::Unsigned64: return "uint64_t";
        case VarType::Real32: return "float";
        case VarType::Real64: return "double";
        case VarType::String: return "string";
        case VarType::Max:
        default: return "Unkwown";
    }
}

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
    BR_PROFILE_FUNCTION();
    for (auto&& request : m_uploadRequestQueue | std::views::filter([](const auto& r) {
                              return r.status() == CanOpen::SdoRequestStatus::Complete;
                          })) {
        auto                result = request.future.get();
        FulfilledSdoRequest requestResult {
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
                  [](const auto& r) { return r.status() == CanOpen::SdoRequestStatus::Complete; });
}

void Sdo::renderUploadRequestMaker(CanOpen::Node& node)
{
    BR_PROFILE_FUNCTION();
    ImGui::SliderInt("Index##upload", &m_uploadRequestIndex, 0, 65535, "0x%04x");
    ImGui::SliderInt("Sub Index##upload", &m_uploadRequestSubIndex, 0, 255, "0x%02x");
    ImGui::SliderInt("Timeout##upload", &m_uploadRequestTimeout, 0, 65535, "%d ms");
    ImGui::Checkbox("Is Block##upload", &m_uploadRequestIsBlock);
    if (ImGui::Button("Send##upload")) {
        m_uploadRequestQueue.push_back(node.sdoInterface()->uploadData(static_cast<uint16_t>(m_uploadRequestIndex),
                                                                       static_cast<uint8_t>(m_uploadRequestSubIndex),
                                                                       static_cast<uint16_t>(m_uploadRequestTimeout),
                                                                       m_uploadRequestIsBlock));
    }
}

void Sdo::renderActiveUploadRequests()
{
    BR_PROFILE_FUNCTION();
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
                   if (request.status() == CanOpen::SdoRequestStatus::OnGoing) {
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
    BR_PROFILE_FUNCTION();
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
               [](Widget::Table& table, const FulfilledSdoRequest& request) {
                   if (request.hasFailed) { ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, s_requestFailedColor); }

                   table.CellContentTextWrapped("0x{:04x}", request.index);
                   table.CellContentTextWrapped("0x{:02x}", request.subIndex);
                   table.CellContentTextWrapped(request.abortCode);
                   table.CellContentTextWrapped(request.result);
               });
}

void Sdo::renderDownloadTab(CanOpen::Node& node)
{
    BR_PROFILE_FUNCTION();
    // Read the completed requests
    purgeCompletedDownloadRequests();

    renderDownloadRequestMaker(node);

    ImGui::Separator();
    renderActiveDownloadRequests();

    ImGui::Separator();
    renderDownloadRequestHistory();
}

void Sdo::purgeCompletedDownloadRequests()
{
    BR_PROFILE_FUNCTION();
    for (auto&& request : m_downloadRequestQueue | std::views::filter([](const auto& r) {
                              return r.status() == CanOpen::SdoRequestStatus::Complete;
                          })) {
        BR_ASSERT(request.future.valid(), "Future isn't valid!");
        auto                result = request.future.get();
        FulfilledSdoRequest requestResult {
          .index     = request.index(),
          .subIndex  = request.subIndex(),
          .hasFailed = result != CO_SDO_RT_ok_communicationEnd,
          .abortCode = std::format("{}", request.abortCode()),
          .result    = std::format("{}", result),
        };

        m_downloadRequestHistory.push_back(requestResult);
    }
    std::erase_if(m_downloadRequestQueue,
                  [](const auto& r) { return r.status() == CanOpen::SdoRequestStatus::Complete; });
}

VarType operator++(VarType& var, int)
{
    VarType old = var;
    var         = static_cast<VarType>(std::to_underlying(var) + 1);
    return old;
}

void Sdo::renderDownloadRequestMaker(CanOpen::Node& node)
{
    ImGui::SliderInt("Index##download", &m_downloadRequestIndex, 0, 65535, "0x%04x");
    ImGui::SliderInt("Sub Index##download", &m_downloadRequestSubIndex, 0, 255, "0x%02x");
    ImGui::SliderInt("Timeout##download", &m_downloadRequestTimeout, 0, 65535, "%d ms");
    ImGui::Checkbox("Is Block##download", &m_downloadRequestIsBlock);
    if (ImGui::BeginCombo("Type##download", toString(m_downloadRequestType).data())) {
        for (VarType type = VarType::Boolean; type < VarType::Max; type++) {
            if (ImGui::Selectable(toString(type).data())) {
                m_downloadRequestType = type;
                switch (type) {
                    case VarType::Boolean: m_downloadRequestData = false; break;
                    case VarType::Signed8: m_downloadRequestData = static_cast<int8_t>(0); break;
                    case VarType::Signed16: m_downloadRequestData = static_cast<int16_t>(0); break;
                    case VarType::Signed32: m_downloadRequestData = static_cast<int32_t>(0); break;
                    case VarType::Signed64: m_downloadRequestData = static_cast<int64_t>(0); break;
                    case VarType::Unsigned8: m_downloadRequestData = static_cast<uint8_t>(0); break;
                    case VarType::Unsigned16: m_downloadRequestData = static_cast<uint16_t>(0); break;
                    case VarType::Unsigned32: m_downloadRequestData = static_cast<uint32_t>(0); break;
                    case VarType::Unsigned64: m_downloadRequestData = static_cast<uint64_t>(0); break;
                    case VarType::Real32: m_downloadRequestData = 0.0f; break;
                    case VarType::Real64: m_downloadRequestData = 0.0; break;
                    case VarType::String:
                        m_downloadRequestData = [] {
                            std::array<char, 128> a {};
                            std::ranges::fill(a, 0);
                            return a;
                        }();
                        break;
                    case VarType::Max: m_downloadRequestData = false; break;
                }
            }
        }
        ImGui::EndCombo();
    }

    auto send = [this, &node]<typename T>() {
        if (ImGui::Button("Send##download")) {
            m_downloadRequestQueue.push_back(
              node.sdoInterface()->downloadData(static_cast<uint16_t>(m_downloadRequestIndex),
                                                static_cast<uint8_t>(m_downloadRequestSubIndex),
                                                std::get<T>(m_downloadRequestData),
                                                m_downloadRequestTimeout,
                                                m_downloadRequestIsBlock));
        }
    };

    Frasy::visit(
      m_downloadRequestData,
      [this, &send](bool& b) {
          ImGui::Checkbox("Flag", &b);
          send.operator()<bool>();
      },
      [this, &send]<typename T>(T& v) {
          T                       min  = std::numeric_limits<T>::lowest();
          T                       max  = std::numeric_limits<T>::max();
          constexpr ImGuiDataType type = ImGuiTypeFromType_v<T>;

          // ImGui gets triggered if the floats and doubles are too big...
          if constexpr (std::same_as<T, float> || std::same_as<T, double> || std::signed_integral<T>) {
              min /= 2;
              max /= 2;
          }

          ImGui::SliderScalar("Value", type, &v, &min, &max);
          send.operator()<T>();
      },
      [this, &send](std::array<char, 128>& v) {
          ImGui::InputText("Value", v.data(), v.size());
          send.operator()<std::array<char, 128>>();
      });
}

void Sdo::renderActiveDownloadRequests()
{
    BR_PROFILE_FUNCTION();
    ImGui::Text("Active Requests");
    Widget::Table("active download requests table", 7, ImVec2 {0.0f, ImGui::GetContentRegionAvail().y / 3})
      .ColumnHeader("Status")
      .ColumnHeader("Index")
      .ColumnHeader("Sub Index")
      .ColumnHeader("Indicated Size")
      .ColumnHeader("Transferred Size")
      .ColumnHeader("Data")
      .ColumnHeader("Cancel")
      .ScrollFreeze()
      .FinishHeader()
      .Content(m_downloadRequestQueue | std::views::reverse,
               [](Widget::Table& table, CanOpen::SdoDownloadDataResult& request) {
                   if (request.status() == CanOpen::SdoRequestStatus::OnGoing) {
                       ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, s_onGoingRequestColor);
                   }

                   table.CellContentTextWrapped(request.status());
                   table.CellContentTextWrapped("0x{:04x}", request.index());
                   table.CellContentTextWrapped("0x{:02x}", request.subIndex());
                   table.CellContentTextWrapped(request.data().size());
                   table.CellContentTextWrapped(request.sizeTransferred());
                   table.CellContentTextWrapped(fmt::format("{:02x}", fmt::join(request.data(), ", ")));
                   if (table.CellContent([] { return ImGui::Button("Cancel"); })) { request.cancel(); }
               });
}

void Sdo::renderDownloadRequestHistory()
{
    BR_PROFILE_FUNCTION();
    ImGui::Text("History");
    ImGui::SameLine();
    if (ImGui::Button("Clear")) { m_downloadRequestHistory.clear(); }
    Widget::Table("download request history", 4)
      .ColumnHeader("Index")
      .ColumnHeader("Sub Index")
      .ColumnHeader("Abort Code")
      .ColumnHeader("Result")
      .ScrollFreeze()
      .FinishHeader()
      .Content(m_downloadRequestHistory | std::views::reverse,
               [](Widget::Table& table, const FulfilledSdoRequest& request) {
                   if (request.hasFailed) { ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, s_requestFailedColor); }

                   table.CellContentTextWrapped("0x{:04x}", request.index);
                   table.CellContentTextWrapped("0x{:02x}", request.subIndex);
                   table.CellContentTextWrapped(request.abortCode);
                   table.CellContentTextWrapped(request.result);
               });
}
}    // namespace Frasy::CanOpenViewer
