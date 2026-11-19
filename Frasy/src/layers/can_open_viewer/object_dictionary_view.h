/**
 * @file    object_dictionary_view.h
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
#ifndef FRASY_LAYERS_CAN_OPEN_VIEWER_OBJECT_DICTIONARY_VIEW_H
#define FRASY_LAYERS_CAN_OPEN_VIEWER_OBJECT_DICTIONARY_VIEW_H

#include "utils/communication/can_open/node.h"
#include "utils/communication/can_open/object_dictionary.h"
#include "utils/communication/can_open/object_dictionary_registry.h"
#include "utils/communication/can_open/services/sdo_downloader.h"
#include "utils/communication/can_open/services/sdo_uploader.h"

#include <array>
#include <memory>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Frasy::CanOpenViewer {

/**
 * @brief Browses the object dictionary of a node, as described by the EDS or XDD of that node.
 *
 * Every entry can be read from, and written to, the device (when its access type allows it) through SDO transfers.
 */
class ObjectDictionaryView {
public:
    explicit ObjectDictionaryView(std::string odPath);

    void onImGuiRender(CanOpen::Node& node);

private:
    enum class Phase : uint8_t {
        Idle,
        Pending,
        Done,
        Failed,
    };

    //! Everything known about one entry (index and sub-index) of the device.
    struct EntryState {
        Phase       readPhase = Phase::Idle;
        //! The value read when the read is done, a short reason when it failed.
        std::string readText;
        //! The long version of the reason of a failure, for a tooltip.
        std::string readDetail;
        Phase       writePhase = Phase::Idle;
        std::string writeText;
        std::string writeDetail;

        std::array<char, 256> editBuffer = {};

        std::optional<CanOpen::SdoUploadDataResult>   readRequest;
        std::optional<CanOpen::SdoDownloadDataResult> writeRequest;
        //! Type used to interpret the answer of the pending read.
        CanOpen::VarType readType = CanOpen::VarType::Undefined;
        //! The pending read is an OCTET_STRING, edited as a list of bytes.
        bool isReadOctetString = false;
    };

    [[nodiscard]] static constexpr uint32_t makeKey(uint16_t index, uint8_t subIndex) noexcept
    {
        return (static_cast<uint32_t>(index) << 8) | subIndex;
    }

    //! Gets the dictionary from the registry: the file is only parsed if nobody did it yet, or when forced.
    void load(bool forceReload = false);

    void pollRequests(CanOpen::Node& node);
    void completeRead(EntryState& state);
    void completeWrite(CanOpen::Node& node, uint32_t key, EntryState& state);

    void requestRead(CanOpen::Node& node, uint16_t index, const CanOpen::OdEntry& entry);
    void requestWrite(CanOpen::Node& node, uint16_t index, const CanOpen::OdEntry& entry);

    void renderToolbar();
    void renderTable(CanOpen::Node& node);
    void renderObject(CanOpen::Node& node, const CanOpen::OdObject& object);
    void renderEntryCells(CanOpen::Node&            node,
                          uint16_t                  index,
                          const CanOpen::OdEntry&   entry,
                          const CanOpen::OdObject&  object);
    void renderEntryTooltip(const CanOpen::OdObject& object, const CanOpen::OdEntry& entry) const;
    //! Draws a name, which opens the comment window when it has a comment.
    void renderName(const std::string& name, const std::string& comment, const std::string& windowTitle);
    void openCommentWindow(const std::string& title, const std::string& comment);
    /**
     * @brief Renders the windows that show the full comment of an entry.
     *
     * Rendered with the page: they only show while the page of this node is the one selected in the node explorer.
     */
    void renderCommentWindows();

    //! Part of the index space, as defined by CiA 301. The indexes are browsed one part at a time.
    struct IndexRange {
        std::string_view name;
        uint16_t         first;
        uint16_t         last;
    };
    static constexpr std::array<IndexRange, 3> s_ranges = {{
      {"Communication Specific Parameters", 0x1000, 0x1FFF},
      {"Manufacturer Specific Parameters", 0x2000, 0x5FFF},
      {"Device Profile Specific Parameters", 0x6000, 0x9FFF},
    }};
    //! Position of the Device Profile Specific Parameters, shown first.
    static constexpr std::size_t s_defaultRange = 2;
    //! Position of the indexes that are in none of the ranges above (data types, reserved indexes).
    static constexpr std::size_t s_otherRange = s_ranges.size();

    [[nodiscard]] static std::size_t rangeOf(uint16_t index) noexcept;
    [[nodiscard]] static std::string rangeName(std::size_t range);

    //! Expands (or folds) every record and array, whatever the selected range.
    void setAllOpen(bool isOpen);

    [[nodiscard]] bool matchesFilter(const CanOpen::OdObject& object) const;
    [[nodiscard]] bool matchesFilter(const CanOpen::OdObject& object, const CanOpen::OdEntry& entry) const;

private:
    static constexpr const char* s_tag = "CANopen OD";

    std::string m_odPath;
    bool        m_loaded = false;
    std::string m_error;
    //! Shared with the Lua environment, which got it from the same registry.
    std::shared_ptr<const CanOpen::ObjectDictionary> m_dictionary;

    std::array<char, 64> m_filter = {};
    //! The filter in lower case, as used by the comparisons.
    std::string          m_lowerFilter;

    std::size_t m_selectedRange = s_defaultRange;
    //! Indexes of the records and arrays that are expanded. Kept here so that they stay as they are when switching range.
    std::unordered_set<uint16_t> m_openObjects;

    int m_timeoutMs = 500;
    int m_retries   = 1;

    std::unordered_map<uint32_t, EntryState> m_states;

    //! Window that shows the whole comment of an entry.
    struct CommentWindow {
        std::string title;
        std::string comment;
        bool        isOpen = true;
    };
    std::vector<CommentWindow> m_commentWindows;
    //! Identifies the node in the title of its windows, as windows of different nodes must not share a title.
    std::string m_windowPrefix;
};

}    // namespace Frasy::CanOpenViewer

#endif    // FRASY_LAYERS_CAN_OPEN_VIEWER_OBJECT_DICTIONARY_VIEW_H
