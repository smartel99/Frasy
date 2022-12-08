/**
 * @file    ScriptEngineRegistry
 * @author  Samuel Martel
 * @p       https://github.com/smartel99
 * @date    10/11/2020 4:55:29 PM
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

/*********************************************************************************************************************/
// [SECTION] Includes
/*********************************************************************************************************************/
#include "ScriptEngineRegistry.h"

namespace Brigerad
{
/*********************************************************************************************************************/
// [SECTION] Private Macro Definitions
/*********************************************************************************************************************/



/*********************************************************************************************************************/
// [SECTION] Private Function Declarations
/*********************************************************************************************************************/

/*********************************************************************************************************************/
// [SECTION] Public Method Definitions
/*********************************************************************************************************************/
void ScriptEngineRegistry::RegisterAllTypes()
{
    RegisterEntity();
    RegisterComponents();
    RegisterVec2();
    RegisterVec3();
    RegisterVec4();
    RegisterMat4();
    RegisterTexture2D();
    RegisterSubTexture2D();
    RegisterDrawQuad();
}

/*********************************************************************************************************************/
// [SECTION] Private Method Definitions
/*********************************************************************************************************************/


/*********************************************************************************************************************/
// [SECTION] Private Function Declarations
/*********************************************************************************************************************/



}    // namespace Brigerad
