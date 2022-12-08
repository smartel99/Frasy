/**
 * @file    ScriptEngineRegistryRenderer
 * @author  Samuel Martel
 * @p       https://github.com/smartel99
 * @date    10/18/2020 3:54:42 PM
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

#include "../Renderer/Renderer2D.h"

#include <glm/glm.hpp>


namespace Brigerad
{
namespace Scripting
{
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

void ScriptEngineRegistry::RegisterDrawQuad()
{
    auto lua = Scripting::GetState();

    auto renderer        = lua->new_usertype<Renderer2D>("Renderer2D", sol::no_constructor);
    renderer["DrawQuad"] = sol::overload(
      static_cast<void (*)(const glm::vec2&, const glm::vec2&, const glm::vec4&)>(
        &Renderer2D::DrawQuad),
      static_cast<void (*)(const glm::vec3&, const glm::vec2&, const glm::vec4&)>(
        &Renderer2D::DrawQuad),
      // Draw Quad Texture2D with pos<glm::vec2>
      [](const glm::vec2& pos, const glm::vec2& size, const Ref<Texture2D>& texture)
      { Renderer2D::DrawQuad(pos, size, texture); },
      [](const glm::vec2&      pos,
         const glm::vec2&      size,
         const Ref<Texture2D>& texture,
         const glm::vec2&      scale) { Renderer2D::DrawQuad(pos, size, texture, scale); },
      [](const glm::vec2&      pos,
         const glm::vec2&      size,
         const Ref<Texture2D>& texture,
         const glm::vec2&      scale,
         const glm::vec4&      tint) { Renderer2D::DrawQuad(pos, size, texture, scale, tint); },
      // Draw Quad Texture2D with pos<glm::vec3>
      [](const glm::vec3& pos, const glm::vec2& size, const Ref<Texture2D>& texture)
      { Renderer2D::DrawQuad(pos, size, texture); },
      [](const glm::vec3&      pos,
         const glm::vec2&      size,
         const Ref<Texture2D>& texture,
         const glm::vec2&      scale) { Renderer2D::DrawQuad(pos, size, texture, scale); },
      [](const glm::vec3&      pos,
         const glm::vec2&      size,
         const Ref<Texture2D>& texture,
         const glm::vec2&      scale,
         const glm::vec4&      tint) { Renderer2D::DrawQuad(pos, size, texture, scale, tint); },
      // Draw Quad SubTexture2D with pos<glm::vec2>
      [](const glm::vec2& pos, const glm::vec2& size, const Ref<SubTexture2D>& texture)
      { Renderer2D::DrawQuad(pos, size, texture); },
      [](const glm::vec2&         pos,
         const glm::vec2&         size,
         const Ref<SubTexture2D>& texture,
         const glm::vec2&         scale) { Renderer2D::DrawQuad(pos, size, texture, scale); },
      [](const glm::vec2&         pos,
         const glm::vec2&         size,
         const Ref<SubTexture2D>& texture,
         const glm::vec2&         scale,
         const glm::vec4&         tint) { Renderer2D::DrawQuad(pos, size, texture, scale, tint); },
      // Draw Quad SubTexture2D with pos<glm::vec3>
      [](const glm::vec3& pos, const glm::vec2& size, const Ref<SubTexture2D>& texture)
      { Renderer2D::DrawQuad(pos, size, texture); },
      [](const glm::vec3&         pos,
         const glm::vec2&         size,
         const Ref<SubTexture2D>& texture,
         const glm::vec2&         scale) { Renderer2D::DrawQuad(pos, size, texture, scale); },
      [](const glm::vec3&         pos,
         const glm::vec2&         size,
         const Ref<SubTexture2D>& texture,
         const glm::vec2&         scale,
         const glm::vec4&         tint) { Renderer2D::DrawQuad(pos, size, texture, scale, tint); });
}

/*********************************************************************************************************************/
// [SECTION] Private Function Declarations
/*********************************************************************************************************************/
}    // namespace Brigerad
