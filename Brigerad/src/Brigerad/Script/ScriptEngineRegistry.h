/**
 * @file    ScriptEngineRegistry.h
 * @author  Samuel Martel
 * @p       https://github.com/smartel99
 * @date    10/11/2020 4:54:53 PM
 *
 * @brief
 ******************************************************************************
 * Copyright (C) 2020  Samuel Martel
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *****************************************************************************/
#pragma once

/*********************************************************************************************************************/
// [SECTION] Includes
/*********************************************************************************************************************/

namespace Brigerad
{
/*********************************************************************************************************************/
// [SECTION] Defines
/*********************************************************************************************************************/


/*********************************************************************************************************************/
// [SECTION] Class Declarations
/*********************************************************************************************************************/
class ScriptEngineRegistry
{
public:
    static void RegisterAllTypes();

private:
    static void RegisterEntity();
    static void RegisterComponents();
    static void RegisterVec2();
    static void RegisterVec3();
    static void RegisterVec4();
    static void RegisterMat4();
    static void RegisterTexture2D();
    static void RegisterSubTexture2D();
    static void RegisterDrawQuad();
};
}    // namespace Brigerad
