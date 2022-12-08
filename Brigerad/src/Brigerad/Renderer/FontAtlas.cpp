/**
 * @file    FontAtlas
 * @author  Samuel Martel
 * @p       https://github.com/smartel99
 * @date    11/27/2020 4:00:58 PM
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
#include "FontAtlas.h"

#include "../../Platform/OpenGL/OpenGLFontAtlas.h"
#include "Renderer.h"



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


/*********************************************************************************************************************/
// [SECTION] Private Method Definitions
/*********************************************************************************************************************/


Ref<FontAtlas> FontAtlas::Create(const std::string& path)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            BR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL: return std::make_shared<OpenGLFontAtlas>(path);
        default: BR_CORE_ASSERT(false, "Invalid RendererAPI!"); return nullptr;
    }
}

/*********************************************************************************************************************/
// [SECTION] Private Function Declarations
/*********************************************************************************************************************/
}    // namespace Brigerad
