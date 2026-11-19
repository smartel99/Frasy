/**
 * @file    object_dictionary_view.cpp
 * @author  Paul Thomas
 * @date    2026-11-19
 * @brief   Page of the node explorer that browses the object dictionary of a node, and reads and writes its entries.
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
#include "object_dictionary_view.h"

#include "utils/communication/can_open/od_value.h"
#include "utils/communication/can_open/to_string.h"
#include "utils/communication/can_open/types.h"
#include "utils/imgui/selector.h"
#include "utils/string_utils.h"

#include <Brigerad/Core/Log.h>
#include <Brigerad/Debug/Instrumentor.h>
#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <chrono>
#include <cstring>
#include <format>
#include <future>
#include <iterator>
#include <span>

namespace Frasy::CanOpenViewer {

using CanOpen::OdAccess;
using CanOpen::OdEntry;
using CanOpen::OdObject;
using CanOpen::VarType;
namespace OdValue = CanOpen::OdValue;

namespace {
using StringUtils::toLower;

constexpr ImVec4 s_okColor    = {0.30f, 0.80f, 0.35f, 1.0f};
constexpr ImVec4 s_errorColor = {0.90f, 0.30f, 0.30f, 1.0f};

//! Raw values longer than this are not copied in the edit field, as they are shown truncated.
constexpr std::size_t s_maxEditableRawBytes = 64;
//! Octet strings longer than this do not fit in the 255 characters of the edit field, at 5 characters per byte.
constexpr std::size_t s_maxEditableOctets = 255 / 5;

bool containsLower(std::string_view text, std::string_view lowerNeedle)
{
    return toLower(text).find(lowerNeedle) != std::string::npos;
}

void copyToBuffer(std::span<char> buffer, std::string_view text)
{
    const auto size = (std::min)(text.size(), buffer.size() - 1);
    std::memcpy(buffer.data(), text.data(), size);
    buffer[size] = '\0';
}

//! Short description of why a transfer failed, small enough to fit in a table cell.
std::string shortFailure(CO_SDO_abortCode_t abortCode)
{
    if (abortCode == CO_SDO_AB_TIMEOUT) { return "Timeout"; }
    if (abortCode != CO_SDO_AB_NONE) { return std::format("Abort 0x{:08X}", static_cast<uint32_t>(abortCode)); }
    return "Failed";
}

std::string longFailure(CO_SDO_abortCode_t abortCode, CO_SDO_return_t returnCode)
{
    if (abortCode != CO_SDO_AB_NONE) {
        return std::format("0x{:08X}: {}", static_cast<uint32_t>(abortCode), CanOpen::toString(abortCode));
    }
    return std::string {CanOpen::toString(returnCode)};
}

std::string objectLabel(uint16_t index)
{
    return std::format("0x{:04X}", index);
}

//! Comments longer than this are cut in the tooltip. The whole text is in the comment window.
constexpr std::size_t s_maxTooltipChars = 300;

//! Shows the beginning of a comment, and tells that the rest is one click away.
void renderCommentPreview(const std::string& comment)
{
    std::string_view preview = comment;
    const bool       isCut   = preview.size() > s_maxTooltipChars;
    if (isCut) {
        // Do not cut a multi-byte UTF-8 character in the middle.
        std::size_t end = s_maxTooltipChars;
        while (end > 0 && (static_cast<unsigned char>(comment[end]) & 0xC0) == 0x80) {
            --end;
        }
        preview = preview.substr(0, end);
    }
    ImGui::TextUnformatted(preview.data(), preview.data() + preview.size());
    if (isCut) { ImGui::TextDisabled("..."); }
    ImGui::TextDisabled("Click the name to open the whole comment.");
}

template<typename Request>
bool isReady(Request& request)
{
    using namespace std::chrono_literals;
    return request.has_value() && request->future.wait_for(0s) == std::future_status::ready;
}
}    // namespace

ObjectDictionaryView::ObjectDictionaryView(std::string odPath) : m_odPath(std::move(odPath))
{
}

void ObjectDictionaryView::onImGuiRender(CanOpen::Node& node)
{
    BR_PROFILE_FUNCTION();
    // The nodes are rebuilt for each run: the same node ID can then be another board, described by another file.
    if (node.odPath() != m_odPath) {
        m_odPath = node.odPath();
        m_openObjects.clear();
        m_loaded = false;
    }
    if (!m_loaded) { load(); }

    m_windowPrefix = std::format("{} (0x{:02X})", node.name(), node.nodeId());

    ImGui::PushID(this);
    pollRequests(node);
    renderToolbar();
    if (m_dictionary) { renderTable(node); }
    ImGui::PopID();

    renderCommentWindows();
}

void ObjectDictionaryView::load(bool forceReload)
{
    m_loaded = true;
    m_states.clear();
    m_dictionary.reset();
    m_error.clear();

    if (m_odPath.empty()) {
        m_error = "No EDS or XDD file is declared for this node.";
        return;
    }

    auto result = CanOpen::ObjectDictionaryRegistry::get().load(m_odPath, forceReload);
    if (!result.has_value()) {
        m_error = result.error();
        BR_LOG_WARN(s_tag, "{}", m_error);
        return;
    }
    m_dictionary = std::move(*result);
}

// --- Requests ---

void ObjectDictionaryView::pollRequests(CanOpen::Node& node)
{
    for (auto& [key, state] : m_states) {
        if (state.readPhase == Phase::Pending && isReady(state.readRequest)) { completeRead(state); }
        if (state.writePhase == Phase::Pending && isReady(state.writeRequest)) { completeWrite(node, key, state); }
    }
}

void ObjectDictionaryView::completeRead(EntryState& state)
{
    const auto result = state.readRequest->future.get();
    if (result.has_value()) {
        state.readPhase  = Phase::Done;
        state.readText   = OdValue::format(state.readType, *result);
        state.readDetail = {};

        // Start from the value of the device when editing it, unless it was truncated for display.
        if (state.isReadOctetString) {
            // Written back as typed: a list of bytes, where hexadecimal needs its 0x prefix.
            if (result->size() <= s_maxEditableOctets) {
                copyToBuffer(state.editBuffer, OdValue::formatOctetString(*result));
            }
        }
        else if (state.readType != VarType::Undefined || result->size() <= s_maxEditableRawBytes) {
            copyToBuffer(state.editBuffer, OdValue::format(state.readType, *result, false));
        }
    }
    else {
        const auto abortCode = state.readRequest->abortCode();
        state.readPhase      = Phase::Failed;
        state.readText       = shortFailure(abortCode);
        state.readDetail     = longFailure(abortCode, result.error());
    }
    state.readRequest.reset();
}

void ObjectDictionaryView::completeWrite(CanOpen::Node& node, uint32_t key, EntryState& state)
{
    const auto returnCode = state.writeRequest->future.get();
    const auto abortCode  = state.writeRequest->abortCode();
    state.writeRequest.reset();

    if (returnCode != CO_SDO_RT_ok_communicationEnd) {
        state.writePhase  = Phase::Failed;
        state.writeText   = shortFailure(abortCode);
        state.writeDetail = longFailure(abortCode, returnCode);
        return;
    }

    state.writePhase  = Phase::Done;
    state.writeText   = "Written";
    state.writeDetail = {};

    // Read the value back, to confirm what the device actually holds.
    const auto index = static_cast<uint16_t>(key >> 8);
    const auto sub   = static_cast<uint8_t>(key & 0xFF);
    if (m_dictionary) {
        if (const auto* object = m_dictionary->findObject(index)) {
            auto it = std::ranges::find(object->entries, sub, &OdEntry::subIndex);
            if (it != object->entries.end() && isReadable(it->access)) { requestRead(node, index, *it); }
        }
    }
}

void ObjectDictionaryView::requestRead(CanOpen::Node& node, uint16_t index, const OdEntry& entry)
{
    auto* sdo = node.sdoInterface();
    if (sdo == nullptr) { return; }

    auto& state = m_states[makeKey(index, entry.subIndex)];
    if (state.readPhase == Phase::Pending) { return; }

    state.readType          = entry.varType;
    state.isReadOctetString = entry.dataType == static_cast<uint16_t>(CanOpen::DataType::octetString);
    state.readPhase         = Phase::Pending;
    state.readText    = {};
    state.readDetail  = {};
    state.readRequest = sdo->uploadData(index,
                                        entry.subIndex,
                                        static_cast<uint16_t>(m_timeoutMs),
                                        static_cast<uint8_t>(m_retries),
                                        false,
                                        entry.varType);
}

void ObjectDictionaryView::requestWrite(CanOpen::Node& node, uint16_t index, const OdEntry& entry)
{
    auto* sdo = node.sdoInterface();
    if (sdo == nullptr) { return; }

    auto& state = m_states[makeKey(index, entry.subIndex)];
    if (state.writePhase == Phase::Pending) { return; }

    auto bytes = OdValue::parse(entry, state.editBuffer.data());
    if (!bytes.has_value()) {
        state.writePhase  = Phase::Failed;
        state.writeText   = "Invalid value";
        state.writeDetail = bytes.error();
        return;
    }

    state.writePhase   = Phase::Pending;
    state.writeText    = {};
    state.writeDetail  = {};
    state.writeRequest = sdo->downloadData(
      index, entry.subIndex, *bytes, static_cast<uint16_t>(m_timeoutMs), static_cast<uint8_t>(m_retries), false);
}

// --- Index ranges ---

std::size_t ObjectDictionaryView::rangeOf(uint16_t index) noexcept
{
    static_assert(s_ranges[s_defaultRange].first == 0x6000, "The default range is the one of the device profile.");
    const auto it = std::ranges::find_if(s_ranges, [index](const IndexRange& r) { return index >= r.first && index <= r.last; });
    return static_cast<std::size_t>(std::distance(s_ranges.begin(), it));    // s_otherRange when not found.
}

std::string ObjectDictionaryView::rangeName(std::size_t range)
{
    if (range >= s_ranges.size()) { return "Other indexes"; }
    return std::format("0x{:04X} - {}", s_ranges[range].first, s_ranges[range].name);
}

// --- Filter ---

bool ObjectDictionaryView::matchesFilter(const OdObject& object) const
{
    if (m_lowerFilter.empty()) { return true; }
    if (containsLower(objectLabel(object.index), m_lowerFilter) || containsLower(object.name, m_lowerFilter)) {
        return true;
    }
    return std::ranges::any_of(object.entries,
                               [this](const OdEntry& entry) { return containsLower(entry.name, m_lowerFilter); });
}

bool ObjectDictionaryView::matchesFilter(const OdObject& object, const OdEntry& entry) const
{
    if (m_lowerFilter.empty()) { return true; }
    // When the object itself is what is searched for, all of its sub-indexes are of interest.
    if (containsLower(objectLabel(object.index), m_lowerFilter) || containsLower(object.name, m_lowerFilter)) {
        return true;
    }
    return containsLower(entry.name, m_lowerFilter);
}

// --- Rendering ---

void ObjectDictionaryView::renderToolbar()
{
    if (m_dictionary) {
        const auto& vendor  = m_dictionary->getVendorName();
        const auto& product = m_dictionary->getProductName();
        ImGui::Text("%s%s%s", product.c_str(), (product.empty() || vendor.empty()) ? "" : " - ", vendor.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", m_odPath.c_str());
        ImGui::SameLine();
    }
    if (ImGui::Button("Reload")) { load(true); }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Reads the EDS or XDD file again."); }

    if (!m_error.empty()) {
        ImGui::TextColored(s_errorColor, "%s", m_error.c_str());
        return;
    }

    // Number of objects in each range, with the filter applied: the labels show where the matches are.
    std::array<std::size_t, s_ranges.size() + 1> counts = {};
    for (const auto& object : m_dictionary->getObjects()) {
        if (matchesFilter(object)) { ++counts[rangeOf(object.index)]; }
    }
    // The other indexes are only offered when the file has some.
    const bool hasOther = std::ranges::any_of(
      m_dictionary->getObjects(), [](const OdObject& object) { return rangeOf(object.index) == s_otherRange; });
    std::vector<std::size_t> ranges = {0, 1, 2};
    if (hasOther) { ranges.push_back(s_otherRange); }
    m_selectedRange = (std::min)(m_selectedRange, ranges.size() - 1);
    Widget::Selector("##range", m_selectedRange, ranges, [&counts](std::size_t range) {
        return std::format("{} ({})", rangeName(range), counts[range]);
    });

    ImGui::SetNextItemWidth(220.0f);
    if (ImGui::InputTextWithHint("##filter", "Filter by index or name", m_filter.data(), m_filter.size())) {
        m_lowerFilter = toLower(m_filter.data());
        // Show the matches that are in the sub-indexes. The user is free to fold them afterwards.
        if (!m_lowerFilter.empty()) {
            for (const auto& object : m_dictionary->getObjects()) {
                if (object.hasSubIndexes() && matchesFilter(object)) { m_openObjects.insert(object.index); }
            }
        }
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90.0f);
    ImGui::InputInt("Timeout (ms)", &m_timeoutMs, 0);
    m_timeoutMs = std::clamp(m_timeoutMs, 10, 65535);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60.0f);
    ImGui::InputInt("Retries", &m_retries, 0);
    m_retries = std::clamp(m_retries, 0, 255);

    // On the far right of the line.
    const auto& style        = ImGui::GetStyle();
    const float expandWidth  = ImGui::CalcTextSize("Expand").x + 2.0f * style.FramePadding.x;
    const float foldWidth    = ImGui::CalcTextSize("Fold").x + 2.0f * style.FramePadding.x;
    const float buttonsWidth = expandWidth + style.ItemSpacing.x + foldWidth;
    ImGui::SameLine((std::max)(ImGui::GetCursorPosX() + style.ItemSpacing.x,
                               ImGui::GetContentRegionMax().x - buttonsWidth));
    if (ImGui::Button("Expand")) { setAllOpen(true); }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Expands every record and array."); }
    ImGui::SameLine();
    if (ImGui::Button("Fold")) { setAllOpen(false); }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Folds every record and array."); }
}

void ObjectDictionaryView::setAllOpen(bool isOpen)
{
    m_openObjects.clear();
    if (!isOpen || !m_dictionary) { return; }
    for (const auto& object : m_dictionary->getObjects()) {
        if (object.hasSubIndexes()) { m_openObjects.insert(object.index); }
    }
}

void ObjectDictionaryView::renderTable(CanOpen::Node& node)
{
    BR_PROFILE_FUNCTION();
    const bool hasObjects = std::ranges::any_of(m_dictionary->getObjects(), [this](const OdObject& object) {
        return rangeOf(object.index) == m_selectedRange && matchesFilter(object);
    });
    if (!hasObjects) {
        ImGui::TextDisabled("No object in this range.");
        return;
    }

    constexpr ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                      ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;
    if (!ImGui::BeginTable("object dictionary", 7, flags, ImVec2 {0.0f, ImGui::GetContentRegionAvail().y})) { return; }

    ImGui::TableSetupColumn("Object", ImGuiTableColumnFlags_WidthFixed, 130.0f);
    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 2.0f);
    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 110.0f);
    ImGui::TableSetupColumn("Access", ImGuiTableColumnFlags_WidthFixed, 55.0f);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 2.0f);
    ImGui::TableSetupColumn("New value", ImGuiTableColumnFlags_WidthStretch, 1.5f);
    ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 230.0f);
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableHeadersRow();

    for (const auto& object : m_dictionary->getObjects()) {
        if (rangeOf(object.index) == m_selectedRange && matchesFilter(object)) { renderObject(node, object); }
    }
    ImGui::EndTable();
}

void ObjectDictionaryView::renderObject(CanOpen::Node& node, const OdObject& object)
{
    constexpr ImGuiTreeNodeFlags leafFlags =
      ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth;
    constexpr ImGuiTreeNodeFlags branchFlags =
      ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    ImGui::PushID(static_cast<int>(object.index));
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    if (!object.hasSubIndexes()) {
        ImGui::TreeNodeEx(objectLabel(object.index).c_str(), leafFlags);
        if (!object.entries.empty()) { renderEntryCells(node, object.index, object.entries.front(), object); }
        ImGui::PopID();
        return;
    }

    // The state is kept here, to be able to expand and fold all the objects.
    const bool wasOpen = m_openObjects.contains(object.index);
    ImGui::SetNextItemOpen(wasOpen, ImGuiCond_Always);
    const bool open = ImGui::TreeNodeEx(objectLabel(object.index).c_str(), branchFlags);
    if (open != wasOpen) {
        if (open) { m_openObjects.insert(object.index); }
        else { m_openObjects.erase(object.index); }
    }

    ImGui::TableNextColumn();
    renderName(object.name, object.description, std::format("{} - {}: {}", m_windowPrefix, objectLabel(object.index), object.name));
    if (ImGui::IsItemHovered() && !object.description.empty()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
        renderCommentPreview(object.description);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(CanOpen::toString(object.type).data());
    ImGui::TableNextColumn();    // Access
    ImGui::TableNextColumn();    // Value
    ImGui::TableNextColumn();    // New value
    ImGui::TableNextColumn();
    if (ImGui::SmallButton("Read all")) {
        for (const auto& entry : object.entries) {
            if (isReadable(entry.access)) { requestRead(node, object.index, entry); }
        }
    }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Reads every readable sub-index of this object."); }

    if (open) {
        for (const auto& entry : object.entries) {
            if (!matchesFilter(object, entry)) { continue; }
            ImGui::PushID(entry.subIndex);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TreeNodeEx(std::format("sub 0x{:02X}", entry.subIndex).c_str(), leafFlags);
            renderEntryCells(node, object.index, entry, object);
            ImGui::PopID();
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void ObjectDictionaryView::renderEntryCells(CanOpen::Node&  node,
                                            uint16_t        index,
                                            const OdEntry&  entry,
                                            const OdObject& object)
{
    auto& state = m_states[makeKey(index, entry.subIndex)];

    // Name
    ImGui::TableNextColumn();
    renderName(entry.name,
               entry.description,
               std::format("{} - {} sub 0x{:02X}: {}", m_windowPrefix, objectLabel(object.index), entry.subIndex, entry.name));
    if (ImGui::IsItemHovered()) { renderEntryTooltip(object, entry); }

    // Type
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(entry.dataTypeName.empty() ? OdValue::toString(entry.varType).data()
                                                      : entry.dataTypeName.c_str());

    // Access
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(CanOpen::toString(entry.access).data());

    // Value
    ImGui::TableNextColumn();
    if (!isReadable(entry.access)) { ImGui::TextDisabled("not readable"); }
    else {
        switch (state.readPhase) {
            case Phase::Idle: ImGui::TextDisabled("-"); break;
            case Phase::Pending: ImGui::TextDisabled("Reading..."); break;
            case Phase::Done: ImGui::TextWrapped("%s", state.readText.c_str()); break;
            case Phase::Failed:
                ImGui::TextColored(s_errorColor, "%s", state.readText.c_str());
                if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", state.readDetail.c_str()); }
                break;
        }
    }

    // New value
    const bool canRead   = isReadable(entry.access);
    const bool canWrite  = isWritable(entry.access);
    bool       submitted = false;
    ImGui::TableNextColumn();
    if (canWrite && entry.varType == VarType::Boolean) {
        // The buffer stays the text form of the value, so that writing it goes through the same path as the others.
        // Also sets it when nothing was read yet, as an empty buffer is not a valid boolean.
        bool isChecked = std::string_view {state.editBuffer.data()} == "true";
        ImGui::Checkbox("##value", &isChecked);
        copyToBuffer(state.editBuffer, isChecked ? "true" : "false");
    }
    else if (canWrite) {
        ImGui::SetNextItemWidth(-FLT_MIN);
        submitted = ImGui::InputText(
          "##value", state.editBuffer.data(), state.editBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue);
    }

    // Actions
    ImGui::TableNextColumn();
    ImGui::BeginDisabled(!canRead || state.readPhase == Phase::Pending);
    if (ImGui::SmallButton("Read")) { requestRead(node, index, entry); }
    ImGui::EndDisabled();
    if (!canRead && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("This entry cannot be read.");
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(!canWrite || state.writePhase == Phase::Pending);
    if (ImGui::SmallButton("Write") || (submitted && canWrite && state.writePhase != Phase::Pending)) {
        requestWrite(node, index, entry);
    }
    ImGui::EndDisabled();
    if (!canWrite && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("This entry cannot be written.");
    }

    ImGui::SameLine();
    switch (state.writePhase) {
        case Phase::Idle: break;
        case Phase::Pending: ImGui::TextDisabled("Writing..."); break;
        case Phase::Done: ImGui::TextColored(s_okColor, "%s", state.writeText.c_str()); break;
        case Phase::Failed:
            ImGui::TextColored(s_errorColor, "%s", state.writeText.c_str());
            if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", state.writeDetail.c_str()); }
            break;
    }
}

void ObjectDictionaryView::renderName(const std::string& name,
                                      const std::string& comment,
                                      const std::string& windowTitle)
{
    if (comment.empty()) {
        ImGui::TextUnformatted(name.c_str());
        return;
    }
    // A selectable, so that it looks clickable when hovered.
    if (ImGui::Selectable(name.empty() ? "(unnamed)" : name.c_str())) { openCommentWindow(windowTitle, comment); }
}

void ObjectDictionaryView::openCommentWindow(const std::string& title, const std::string& comment)
{
    // Already open: bring it back to the front instead of opening it twice.
    if (std::ranges::any_of(m_commentWindows, [&title](const CommentWindow& w) { return w.title == title; })) {
        ImGui::SetWindowFocus(title.c_str());
        return;
    }
    m_commentWindows.push_back({.title = title, .comment = comment, .isOpen = true});
}

void ObjectDictionaryView::renderCommentWindows()
{
    for (auto& window : m_commentWindows) {
        ImGui::SetNextWindowSize(ImVec2 {480.0f, 260.0f}, ImGuiCond_FirstUseEver);
        // The window scrolls and is resizable by default, the close button comes from the open flag.
        if (ImGui::Begin(window.title.c_str(), &window.isOpen, ImGuiWindowFlags_NoDocking)) {
            ImGui::TextWrapped("%s", window.comment.c_str());
        }
        ImGui::End();
    }
    std::erase_if(m_commentWindows, [](const CommentWindow& w) { return !w.isOpen; });
}

void ObjectDictionaryView::renderEntryTooltip(const OdObject& object, const OdEntry& entry) const
{
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
    if (!entry.description.empty()) {
        renderCommentPreview(entry.description);
        ImGui::Separator();
    }
    ImGui::Text("0x%04X sub 0x%02X", object.index, entry.subIndex);
    if (!entry.dataTypeName.empty()) { ImGui::Text("Data type: %s", entry.dataTypeName.c_str()); }
    ImGui::Text("Access: %s", CanOpen::toString(entry.access).data());
    if (!entry.defaultValue.empty()) { ImGui::Text("Default: %s", entry.defaultValue.c_str()); }
    if (entry.isPdoMappable) { ImGui::TextUnformatted("Can be mapped in a PDO"); }
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

}    // namespace Frasy::CanOpenViewer
