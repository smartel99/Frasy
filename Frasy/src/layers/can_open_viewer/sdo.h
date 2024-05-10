/**
 * @file    sdo.h
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


#ifndef FRASY_SRC_LAYERS_CAN_OPEN_VIEWER_SDO_H
#define FRASY_SRC_LAYERS_CAN_OPEN_VIEWER_SDO_H

#include "utils/communication/can_open/node.h"

#include <cstdint>
#include <string>
#include <variant>

namespace Frasy::CanOpenViewer {

enum class VarType {
    Boolean = 0,
    Signed8,
    Signed16,
    Signed32,
    Signed64,
    Unsigned8,
    Unsigned16,
    Unsigned32,
    Unsigned64,
    Real32,
    Real64,
    String,
    Max
};


struct FulfilledSdoRequest {
    uint16_t    index     = 0;
    uint8_t     subIndex  = 0;
    bool        hasFailed = false;
    std::string abortCode;
    std::string result;
};

class Sdo {
public:
    explicit Sdo(uint8_t nodeId);

    void onImGuiRender(CanOpen::Node& node);

private:
    void renderUploadTab(CanOpen::Node& node);
    void purgeCompletedUploadRequests();
    void renderUploadRequestMaker(CanOpen::Node& node);
    void renderActiveUploadRequests();
    void renderUploadRequestHistory();

    void renderDownloadTab(CanOpen::Node& node);
    void purgeCompletedDownloadRequests();
    void renderDownloadRequestMaker(CanOpen::Node& node);
    void renderActiveDownloadRequests();
    void renderDownloadRequestHistory();

private:
    std::string m_tabBarName;

    int  m_uploadRequestIndex    = 0;
    int  m_uploadRequestSubIndex = 0;
    int  m_uploadRequestTimeout  = 1000;
    bool m_uploadRequestIsBlock  = false;

    std::vector<CanOpen::SdoUploadDataResult> m_uploadRequestQueue;
    std::vector<FulfilledSdoRequest>          m_uploadRequestHistory;

    int     m_downloadRequestIndex    = 0;
    int     m_downloadRequestSubIndex = 0;
    int     m_downloadRequestTimeout  = 1000;
    bool    m_downloadRequestIsBlock  = false;
    VarType m_downloadRequestType     = VarType::Boolean;
std::variant<bool,
             int8_t,
             int16_t,
             int32_t,
             int64_t,
             uint8_t,
             uint16_t,
             uint32_t,
             uint64_t,
             float,
             double,
             std::array<char, 128>>
  m_downloadRequestData = false;

    std::vector<CanOpen::SdoDownloadDataResult> m_downloadRequestQueue;
    std::vector<FulfilledSdoRequest>            m_downloadRequestHistory;

    static constexpr uint32_t s_requestFailedColor  = 0x203030DC;    // Pretty red uwu.
    static constexpr uint32_t s_onGoingRequestColor = 0x205CFDEC;    // Yellow!
};

}    // namespace Frasy::CanOpenViewer

#endif    // FRASY_SRC_LAYERS_CAN_OPEN_VIEWER_SDO_H
