/**
 * @file    FontAtlas
 * @author  Samuel Martel
 * @p       https://github.com/smartel99
 * @date    11/27/2020 3:55:21 PM
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
#include "Brigerad/Renderer/Texture.h"
#include "Brigerad/Renderer/SubTexture2D.h"
#include "Brigerad/Core/Core.h"

#include <string>
#include "glm/glm.hpp"
/*********************************************************************************************************************/
// [SECTION] Defines
/*********************************************************************************************************************/


/*********************************************************************************************************************/
// [SECTION] Class Declarations
/*********************************************************************************************************************/
namespace Brigerad
{
struct FontGlyph
{
public:
    FontGlyph() = default;
    FontGlyph(Ref<SubTexture2D> texture,
              const glm::vec2&  size,
              const glm::vec2&  offset,
              float             advance)
    : m_texture(texture), m_size(size), m_textureOffset(offset), m_advance(advance)
    {
    }

    Ref<SubTexture2D> m_texture       = nullptr;
    glm::vec2         m_size          = {0.0f, 0.0f};
    glm::vec2         m_textureOffset = {0.0f, 0.0f};
    glm::vec2         m_offset        = {0.0f, 0.0f};
    float             m_lead          = 0.0f;
    float             m_advance       = 0.0f;
    char              m_char          = '\0';
};

class FontAtlas
{
public:
    virtual ~FontAtlas() = default;

    virtual const FontGlyph& GetCharacterTexture(char c) const = 0;
    virtual Ref<Texture2D>   GetFontMap() const                = 0;

    static Ref<FontAtlas> Create(const std::string& path);
};
}    // namespace Brigerad
