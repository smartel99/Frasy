/**
 * @file    var_type.h
 * @author  Paul Thomas
 * @date    3/31/2025
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

#ifndef VAR_TYPE_H
#define VAR_TYPE_H

namespace Frasy::CanOpen {
enum class VarType {
    Undefined = 0,
    Boolean,
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
}    // namespace Frasy::CanOpen

#endif    // VAR_TYPE_H
