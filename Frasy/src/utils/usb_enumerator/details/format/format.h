/**
 * @file    format.h
 * @author  Sam Martel
 * @date    2026-02-27
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


#ifndef FRASY_UTILS_USB_ENUMERATOR_DETAILS_FORMAT_FORMAT_H
#define FRASY_UTILS_USB_ENUMERATOR_DETAILS_FORMAT_FORMAT_H

#include "usb_connection_status.h"

#include <format>

namespace Frasy::Usb {
template<typename T>
struct Formatter {
    template<typename ParseContext>
    constexpr ParseContext::iterator parse(ParseContext& ctx)
    {
        auto it = ctx.begin();
        if (it == ctx.end()) { return it; }
        if (it != ctx.end() && *it != '}') { throw std::format_error("Invalid format args."); }

        return it;
    }

    template<typename FmtContext>
    FmtContext::iterator format(const T& t, FmtContext& ctx) const
    {
        using Usb::toString;
        std::format_to(ctx.out(), "{}", toString(t));

        return ctx.out();
    }
};

template<typename T>
concept Formattable = requires(T t) {
    {
        Usb::toString(t)
    } -> std::same_as<std::string_view>;
};
}

template<Frasy::Usb::Formattable T>
struct std::formatter<T> : Frasy::Usb::Formatter<T> {};


#endif //FRASY_UTILS_USB_ENUMERATOR_DETAILS_FORMAT_FORMAT_H
