/**
 * @file    run_owner.h
 * @author  Frasy
 * @date    2026-08-17
 * @brief   RunOwner enum — tracks who initiated the current test run.
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 */
#ifndef FRASY_UTILS_RUN_OWNER_H
#define FRASY_UTILS_RUN_OWNER_H

namespace Frasy {

/// Identifies who initiated the current test run.
enum class RunOwner {
    None,    ///< No run active
    Gui,     ///< Run initiated by the GUI operator
    Mcp,     ///< Run initiated by an MCP agent
};

}    // namespace Frasy

#endif    // FRASY_UTILS_RUN_OWNER_H
