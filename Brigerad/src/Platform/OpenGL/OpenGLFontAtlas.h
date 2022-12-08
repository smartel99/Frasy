/**
 * @file    OpenGLFontAtlas
 * @author  Samuel Martel
 * @p       https://github.com/smartel99
 * @date    11/27/2020 4:02:55 PM
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
#include "Brigerad/Renderer/FontAtlas.h"

#include <vector>

/*********************************************************************************************************************/
// [SECTION] Defines
/*********************************************************************************************************************/

namespace Brigerad
{
/*********************************************************************************************************************/
// [SECTION] Class Declarations
/*********************************************************************************************************************/
class OpenGLFontAtlas : public FontAtlas
{
public:
    OpenGLFontAtlas(const std::string& fontPath);
    virtual ~OpenGLFontAtlas() override = default;

    virtual const FontGlyph& GetCharacterTexture(char c) const override;
    virtual Ref<Texture2D>   GetFontMap() const override { return m_fontMap; }

private:
    std::vector<FontGlyph> m_atlas;
    Ref<Texture2D>         m_fontMap;    // Texture containing all glyphs.
};
}    // namespace Brigerad
