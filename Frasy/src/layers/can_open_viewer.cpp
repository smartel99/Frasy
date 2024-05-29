/**
 * @file    can_open_viewer.cpp
 * @author  Samuel Martel
 * @date    2024-04-29
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
#include "can_open_viewer.h"

#include "utils/imgui/table.h"

namespace Frasy::CanOpenViewer {

Layer::Layer(CanOpen::CanOpen& canOpen) noexcept : m_canOpen(canOpen)
{
}

void Layer::onAttach()
{
    Brigerad::Layer::onAttach();
}

void Layer::onDetach()
{
    Brigerad::Layer::onDetach();
}

void Layer::onUpdate(Brigerad::Timestep timestep)
{
    Brigerad::Layer::onUpdate(timestep);
}

void Layer::onImGuiRender()
{
    if (!m_isVisible) { return; }
    BR_PROFILE_FUNCTION();

    if (ImGui::Begin(s_windowName, &m_isVisible, ImGuiWindowFlags_NoDocking)) {
        ImGui::Text("%s", m_canOpen.isOpen() ? "Active" : "Innactive");
        ImGui::SameLine();
        if (ImGui::Button("Restart")) { m_canOpen.reset(); }
        if (ImGui::Button("Error Generator")) { m_shouldRenderErrorGenerator = true; }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("!!! CAUTION !!! This menu is not for you to use, you do not need to go there!");
        }
        renderNodes();
    }
    ImGui::End();

    if (m_shouldRenderErrorGenerator) { renderErrorGenerator(); }

    if (m_openNodes.empty()) { return; }

    bool open = true;
    if (ImGui::Begin("Node Explorer", &open)) {
        if (ImGui::BeginTabBar("nodeExplorerTabBar")) {
            for (auto&& node : m_openNodes) {
                node.onImGuiRender();
            }
            ImGui::EndTabBar();
        }

        std::erase_if(m_openNodes, [](const auto& node) { return !node.open(); });
    }
    ImGui::End();
    if (!open) { m_openNodes.clear(); }
}

void Layer::setVisibility(bool visibility)
{
    m_isVisible = visibility;
    ImGui::SetWindowFocus(s_windowName);
}

void Layer::renderNodes()
{
    BR_PROFILE_FUNCTION();

    Widget::Table("nodes", 3)
      .ColumnHeader("Name", ImGuiTableColumnFlags_DefaultSort)
      .ColumnHeader("Node ID")
      .ColumnHeader("State", ImGuiTableColumnFlags_WidthStretch)
      .ScrollFreeze()
      .FinishHeader()
      .Content(m_canOpen.getNodes(), [this](Widget::Table& table, const CanOpen::Node& node) {
          bool clicked    = false;
          auto renderCell = [&clicked](std::string_view str) {
              if (ImGui::Selectable(str.data())) { clicked = true; }
              if (ImGui::IsItemHovered()) {
                  ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
              }
          };

          table.CellContent(renderCell, node.name());
          table.CellContent(renderCell, std::format("{}", node.nodeId()));
          table.CellContent(renderCell,
                            std::format("HB: {}, NMT: {}",
                                        CanOpen::toString(node.getHbState()),
                                        CanOpen::toString(node.getNmtState())));

          if (clicked && !std::ranges::any_of(
                           m_openNodes, [nodeId = node.nodeId()](const auto& n) { return n.nodeId() == nodeId; })) {
              m_openNodes.emplace_back(node.nodeId(), &m_canOpen);
          }
      });
}

void Layer::renderErrorGenerator()
{
    // clang-format off
    static constexpr std::array s_errorKinds = {
      CO_EM_NO_ERROR, CO_EM_CAN_BUS_WARNING, CO_EM_RXMSG_WRONG_LENGTH, CO_EM_RXMSG_OVERFLOW, CO_EM_RPDO_WRONG_LENGTH,
      CO_EM_RPDO_OVERFLOW, CO_EM_CAN_RX_BUS_PASSIVE, CO_EM_CAN_TX_BUS_PASSIVE, CO_EM_NMT_WRONG_COMMAND, CO_EM_TIME_TIMEOUT,
      CO_EM_0A_unused, CO_EM_0B_unused, CO_EM_0C_unused, CO_EM_0D_unused, CO_EM_0E_unused, CO_EM_0F_unused, CO_EM_10_unused,
      CO_EM_11_unused, CO_EM_CAN_TX_BUS_OFF, CO_EM_CAN_RXB_OVERFLOW, CO_EM_CAN_TX_OVERFLOW, CO_EM_TPDO_OUTSIDE_WINDOW,
      CO_EM_16_unused, CO_EM_RPDO_TIME_OUT, CO_EM_SYNC_TIME_OUT, CO_EM_SYNC_LENGTH, CO_EM_PDO_WRONG_MAPPING,
      CO_EM_HEARTBEAT_CONSUMER, CO_EM_HB_CONSUMER_REMOTE_RESET, CO_EM_1D_unused, CO_EM_1E_unused, CO_EM_1F_unused,
      CO_EM_EMERGENCY_BUFFER_FULL, CO_EM_21_unused, CO_EM_MICROCONTROLLER_RESET, CO_EM_23_unused, CO_EM_24_unused,
      CO_EM_25_unused, CO_EM_26_unused, CO_EM_NON_VOLATILE_AUTO_SAVE, CO_EM_WRONG_ERROR_REPORT, CO_EM_ISR_TIMER_OVERFLOW,
      CO_EM_MEMORY_ALLOCATION_ERROR, CO_EM_GENERIC_ERROR, CO_EM_GENERIC_SOFTWARE_ERROR, CO_EM_INCONSISTENT_OBJECT_DICT,
      CO_EM_CALCULATION_OF_PARAMETERS, CO_EM_NON_VOLATILE_MEMORY, CO_EM_MANUFACTURER_START,
    };
    static constexpr std::array s_errorCodes = {
        CO_EMC_NO_ERROR, CO_EMC_GENERIC, CO_EMC_CURRENT, CO_EMC_CURRENT_INPUT, CO_EMC_CURRENT_INSIDE, CO_EMC_CURRENT_OUTPUT,
        CO_EMC_VOLTAGE, CO_EMC_VOLTAGE_MAINS, CO_EMC_VOLTAGE_INSIDE, CO_EMC_VOLTAGE_OUTPUT, CO_EMC_TEMPERATURE,
        CO_EMC_TEMP_AMBIENT, CO_EMC_TEMP_DEVICE, CO_EMC_HARDWARE, CO_EMC_SOFTWARE_DEVICE, CO_EMC_SOFTWARE_INTERNAL,
        CO_EMC_SOFTWARE_USER, CO_EMC_DATA_SET, CO_EMC_ADDITIONAL_MODUL, CO_EMC_MONITORING, CO_EMC_COMMUNICATION,
        CO_EMC_CAN_OVERRUN, CO_EMC_CAN_PASSIVE, CO_EMC_HEARTBEAT, CO_EMC_BUS_OFF_RECOVERED, CO_EMC_CAN_ID_COLLISION,
        CO_EMC_PROTOCOL_ERROR, CO_EMC_PDO_LENGTH, CO_EMC_PDO_LENGTH_EXC, CO_EMC_DAM_MPDO, CO_EMC_SYNC_DATA_LENGTH,
        CO_EMC_RPDO_TIMEOUT, CO_EMC_EXTERNAL_ERROR, CO_EMC_ADDITIONAL_FUNC, CO_EMC_DEVICE_SPECIFIC, CO_EMC401_OUT_CUR_HI,
        CO_EMC401_OUT_SHORTED, CO_EMC401_OUT_LOAD_DUMP, CO_EMC401_IN_VOLT_HI, CO_EMC401_IN_VOLT_LOW, CO_EMC401_INTERN_VOLT_HI,
        CO_EMC401_INTERN_VOLT_LO, CO_EMC401_OUT_VOLT_HIGH, CO_EMC401_OUT_VOLT_LOW,
    };
    // clang-format on

    if (ImGui::Begin("Error Generator", &m_shouldRenderErrorGenerator)) {
        ImGui::Text("!!! CAUTION !!! This menu is not for you to use, you do not need to come here!");
        ImGui::Text("You might break stuff, use at your own risks.");

        if (ImGui::BeginCombo("Error Kind", CanOpen::toString(m_selectedErrorKind).data())) {
            for (auto&& kind : s_errorKinds) {
                if (ImGui::Selectable(CanOpen::toString(kind).data(), kind == m_selectedErrorKind)) {
                    m_selectedErrorKind = kind;
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("Error Code", CanOpen::toString(m_selectedErrorCode).data())) {
            for (auto&& code : s_errorCodes) {
                if (ImGui::Selectable(CanOpen::toString(code).data(), code == m_selectedErrorCode)) {
                    m_selectedErrorCode = code;
                }
            }
            ImGui::EndCombo();
        }

        ImGui::InputText("Error Info", m_selectedErrorInfo.data(), m_selectedErrorInfo.size());

        ImGui::Checkbox("Is Active", &m_selectedErrorIsActive);

        if (ImGui::Button("Send")) {
            uint32_t value = 0;
            if (std::from_chars(
                  m_selectedErrorInfo.data(), m_selectedErrorInfo.data() + m_selectedErrorInfo.size(), value, 16)
                  .ec == std::errc {}) {
                if (m_selectedErrorIsActive) { m_canOpen.reportError(m_selectedErrorKind, m_selectedErrorCode, value); }
                else {
                    m_canOpen.clearError(m_selectedErrorKind, m_selectedErrorCode);
                }
            }
            else {
                BR_LOG_ERROR("CANopen", "Unable to convert '{}' to uint32_t!", fmt::join(m_selectedErrorInfo, ""));
            }
        }
    }
    ImGui::End();
}
}    // namespace Frasy::CanOpenViewer
