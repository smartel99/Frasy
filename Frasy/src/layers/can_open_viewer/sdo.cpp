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
 * not, see <https://www.gnu.org/licenses/>.
 */
#include "sdo.h"

#include "utils/imgui/table.h"
#include "utils/misc/visit.h"

#include <Brigerad/Debug/Instrumentor.h>
#include <cstdint>
#include <format>
#include <imgui.h>

namespace Frasy::CanOpenViewer {

using VarType = CanOpen::VarType;
CanOpen::VarType operator++(CanOpen::VarType& var, int)
{
    VarType old = var;
    var         = static_cast<VarType>(std::to_underlying(var) + 1);
    return old;
}

constexpr std::string_view toString(CanOpen::VarType type)
{
    switch (type) {
        case VarType::Undefined: return "undefined";
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

namespace {
auto _resultArrayToString(const std::span<uint8_t>& result) -> std::string
{
    return std::format("{::02x}", result);
}
auto _resultArrayAsString(const std::span<uint8_t>& result) -> std::string
{
    return std::string(result.begin(), result.end());
}
template<typename T>
auto _resultToString(const std::span<uint8_t>& result) -> std::string
{
    if (result.size() != sizeof(T)) { return fmt::format("Invalid size ({})", result.size()); }
    std::array<uint8_t, sizeof(T)> a {};
    std::copy_n(result.data(), sizeof(T), a.data());
    T v = std::bit_cast<T, std::array<uint8_t, sizeof(T)>>(a);
    return fmt::format("{}", v);
}
auto resultToString(const auto& request, const auto& result) -> std::string
{
    const std::span<uint8_t> resultSpan = result.value();
    switch (request.varType()) {
        case VarType::Boolean: return _resultToString<bool>(result.value());
        case VarType::Signed8: return _resultToString<int8_t>(result.value());
        case VarType::Signed16: return _resultToString<int16_t>(result.value());
        case VarType::Signed32: return _resultToString<int32_t>(result.value());
        case VarType::Signed64: return _resultToString<int64_t>(result.value());
        case VarType::Unsigned8: return _resultToString<uint8_t>(result.value());
        case VarType::Unsigned16: return _resultToString<uint16_t>(result.value());
        case VarType::Unsigned32: return _resultToString<uint32_t>(result.value());
        case VarType::Unsigned64: return _resultToString<uint64_t>(result.value());
        case VarType::Real32: return _resultToString<float>(result.value());
        case VarType::Real64: return _resultToString<double>(result.value());
        case VarType::String: return _resultArrayAsString(result.value());
        default: return _resultArrayToString(resultSpan);
    }
}
};    // namespace

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

        if (result.has_value()) { requestResult.result = resultToString(request, result); }
        else {
            requestResult.result = std::format("Request Failed: {}", result.error());
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
    ImGui::SliderInt("Retries##upload", &m_uploadRequestTries, 0, 255, "%d");
    if (ImGui::BeginCombo("Type##upload", toString(m_uploadRequestType).data())) {
        for (VarType type = VarType::Undefined; type < VarType::Max; type++) {
            if (ImGui::Selectable(toString(type).data())) { m_uploadRequestType = type; }
        }
        ImGui::EndCombo();
    }
    ImGui::Checkbox("Is Block##upload", &m_uploadRequestIsBlock);
    if (ImGui::Button("Send##upload")) {
        m_uploadRequestQueue.push_back(node.sdoInterface()->uploadData(static_cast<uint16_t>(m_uploadRequestIndex),
                                                                       static_cast<uint8_t>(m_uploadRequestSubIndex),
                                                                       static_cast<uint16_t>(m_uploadRequestTimeout),
                                                                       m_uploadRequestTries,
                                                                       m_uploadRequestIsBlock,
                                                                       m_uploadRequestType));
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


void Sdo::renderDownloadRequestMaker(CanOpen::Node& node)
{
    ImGui::SliderInt("Index##download", &m_downloadRequestIndex, 0, 65535, "0x%04x");
    ImGui::SliderInt("Sub Index##download", &m_downloadRequestSubIndex, 0, 255, "0x%02x");
    ImGui::SliderInt("Timeout##download", &m_downloadRequestTimeout, 0, 65535, "%d ms");
    ImGui::SliderInt("Retries##download", &m_downloadRequestTries, 0, 255, "%d");
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
        auto handle = [this, &node]<typename TT>(const TT& data) {
            if (ImGui::Button("Send##download")) {
                m_downloadRequestQueue.push_back(
                  node.sdoInterface()->downloadData(static_cast<uint16_t>(m_downloadRequestIndex),
                                                    static_cast<uint8_t>(m_downloadRequestSubIndex),
                                                    data,
                                                    m_downloadRequestTimeout,
                                                    m_downloadRequestTries,
                                                    m_downloadRequestIsBlock));
            }
        };

        using DataArray = std::array<char, 128>;
        if constexpr (std::same_as<T, DataArray>) {
            const auto&          data = std::get<DataArray>(m_downloadRequestData);
            std::vector<uint8_t> vec;
            vec.reserve(data.size());
            const auto* ptr = data.data();
            while (*ptr != 0) {
                vec.push_back(*ptr);
                ptr++;
            }
            vec.push_back(0);
            handle(vec);
        }
        else {
            handle(std::get<T>(m_downloadRequestData));
        }
    };

    Frasy::visit(
      m_downloadRequestData,
      [this, &send](bool& b) {
          ImGui::Checkbox("Flag", &b);
          send.operator()<bool>();
      },
      [this, &send]<typename T>(T& v) {
          auto vs = std::to_string(v);
          std::copy_n(vs.data(), (std::min)(s_downloadVariableBufferSize, vs.size()), m_downloadVariableBuffer.data());
          if (vs.size() < s_downloadVariableBufferSize) { m_downloadVariableBuffer[vs.size()] = 0; }
          else {
              m_downloadVariableBuffer.back() = 0;
          }
          if (std::is_integral_v<T>) { ImGui::Checkbox("Hex", &m_downloadVariableHex); }
          if (ImGui::InputText("Value", m_downloadVariableBuffer.data(), s_downloadVariableBufferSize)) {
              auto base = m_downloadVariableHex ? 16 : 10;
              try {
                  if constexpr (std::is_integral_v<T>) {
                      if (std::is_signed_v<T>) { v = std::stoll(m_downloadVariableBuffer.data(), nullptr, base); }
                      else if (std::is_unsigned_v<T>) {
                          v = std::stoull(m_downloadVariableBuffer.data(), nullptr, base);
                      }
                  }
                  else if (std::is_floating_point_v<T>) {
                      v = std::stod(m_downloadVariableBuffer.data());
                  }
                  else {
                      BR_APP_ERROR("Cannot parse value");
                  }
              }
              catch (const std::exception& e) {
                  BR_APP_WARN("Failed to parse value");
              }
          }
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
                   table.CellContentTextWrapped(std::format("{::02x}", request.data()));
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
