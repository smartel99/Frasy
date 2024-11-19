/**
 * @file    ScriptEngineRegistryTextures
 * @author  Samuel Martel
 * @p       https://github.com/smartel99
 * @date    10/18/2020 3:52:07 PM
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

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "../Renderer/SubTexture2D.h"
#include "../Renderer/Texture.h"

#include <string>

namespace Brigerad {
namespace Scripting {
extern sol::state* GetState();
}
/*********************************************************************************************************************/
// [SECTION] Private Macro Definitions
/*********************************************************************************************************************/


/*********************************************************************************************************************/
// [SECTION] Private Function Declarations
/*********************************************************************************************************************/


/*********************************************************************************************************************/
// [SECTION] Public Method Definitions
/*********************************************************************************************************************/

/*********************************************************************************************************************/
// [SECTION] Private Method Definitions
/*********************************************************************************************************************/
void ScriptEngineRegistry::RegisterTexture2D()
{
    auto lua = Scripting::GetState();

    auto texture2D = lua->new_usertype<Texture2D>("Texture2D", sol::no_constructor);
    texture2D["Create"] =
      sol::overload(static_cast<Ref<Texture2D> (*)(std::string_view)>(&Texture2D::Create),
                    static_cast<Ref<Texture2D> (*)(uint32_t, uint32_t, uint8_t)>(&Texture2D::Create));
    texture2D["GetWidth"]    = &Texture2D::GetWidth;
    texture2D["GetHeight"]   = &Texture2D::GetHeight;
    texture2D["GetFormat"]   = &Texture2D::GetFormat;
    texture2D["GetFilePath"] = &Texture2D::GetFilePath;
}

void ScriptEngineRegistry::RegisterSubTexture2D()
{
    auto lua = Scripting::GetState();

    auto subTexture2D                = lua->new_usertype<SubTexture2D>("SubTexture2D", sol::no_constructor);
    subTexture2D["CreateFromCoords"] = &SubTexture2D::CreateFromCoords;
    subTexture2D["GetTexture"]       = &SubTexture2D::GetTexture;
    subTexture2D["GetTexCoords"]     = &SubTexture2D::GetTexCoords;
}


/*********************************************************************************************************************/
// [SECTION] Private Function Declarations
/*********************************************************************************************************************/
}    // namespace Brigerad
