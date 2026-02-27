/**
 * @file    oops.h
 * @author  Sam Martel
 * @date    2026-02-26
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DETAILS_OOPS_H
#define FRASY_UTILS_USB_ENUMERATOR_DETAILS_OOPS_H

#include <Brigerad/Core/Log.h>

#include <strsafe.h>

namespace Frasy::Usb::Details {
#define FRASY_USB_OOPS() ::Frasy::Usb::Details::Oops(__FILE__, __LINE__)

inline void Oops(const char* file, ULONG line)
{
    char   szBuf[1024];
    LPTSTR lpMsgBuf;
    DWORD  dwGLE = GetLastError();

    memset(szBuf, 0, sizeof(szBuf));

    // get the system message for this error code
    if (FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr,
        dwGLE,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        // Default language
        (LPTSTR)&lpMsgBuf,
        0,
        nullptr)) {
        StringCchPrintfA(szBuf,
                        sizeof(szBuf),
                        "File: %s, Line %d\r\nGetLastError 0x%x %u %s\n",
                        file,
                        line,
                        dwGLE,
                        dwGLE,
                        lpMsgBuf);
        }
    else {
        StringCchPrintfA(szBuf,
                        sizeof(szBuf),
                        "File: %s, Line %d\r\nGetLastError 0x%x %u\r\n",
                        file,
                        line,
                        dwGLE,
                        dwGLE);
    }
    BR_LOG_ERROR("UsbEnum", "{}", szBuf);

    // Free the system allocated local buffer
    LocalFree(lpMsgBuf);

    return;
}
}

#endif //FRASY_UTILS_USB_ENUMERATOR_DETAILS_OOPS_H
