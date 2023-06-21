/**
 * @file    OpenGLFontAtlas
 * @author  Samuel Martel
 * @p       https://github.com/smartel99
 * @date    11/27/2020 4:12:20 PM
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
#include "OpenGLFontAtlas.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <imstb_truetype.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <array>
#include <fstream>

namespace Brigerad
{
/*********************************************************************************************************************/
// [SECTION] Private Macro Definitions
/*********************************************************************************************************************/
// struct PreTextureGlyph
//{
//    uint32_t* data = nullptr;        // Bitmap data.
//    int       w    = 0;              // Width.
//    int       h    = 0;              // Height.
//    uint8_t   c    = '\0';           // Character it represents.
//    int       xoff = 0, yoff = 0;    // Offset in the texture map.
//    int       advance         = 0;
//    int       leftSideBearing = 0;
//
//    bool operator>(const PreTextureGlyph& other) const { return (w * h) > (other.w * other.h); }
//};

/*********************************************************************************************************************/
// [SECTION] Private Function Declarations
/*********************************************************************************************************************/


/*********************************************************************************************************************/
// [SECTION] Public Method Definitions
/*********************************************************************************************************************/


/*********************************************************************************************************************/
// [SECTION] Private Method Definitions
/*********************************************************************************************************************/


/*********************************************************************************************************************/
// [SECTION] Private Function Declarations
/*********************************************************************************************************************/

OpenGLFontAtlas::OpenGLFontAtlas(const std::string& fontPath)
{
    // Load font file as a bitmap
    uint8_t* fontFile = new uint8_t[1 << 20];

    BR_CORE_ASSERT((fontFile != nullptr), "Unable to allocate memory for fonts");

    fread(fontFile, 1, 1 << 20, fopen(fontPath.c_str(), "rb"));

    stbtt_fontinfo font;
    [[maybe_unused]] int result = stbtt_InitFont(&font, fontFile, stbtt_GetFontOffsetForIndex(fontFile, 0));
    BR_CORE_ASSERT(result != 0,
                   "stbtt_InitFont returned 0");

    int textureWidth  = 8704;
    int textureHeight = 256;
    int lineHeight    = 128;

    // Create a bitmap for the glyphs.
    uint8_t* bitmap = new uint8_t[(size_t)textureWidth * textureHeight];

    // Calculate font scaling.
    float scale = stbtt_ScaleForPixelHeight(&font, (float)lineHeight);

    int x = 0;
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);

    ascent  = (int)roundf(ascent * scale);
    descent = (int)roundf(descent * scale);

    int lowest = 0;

    m_atlas.reserve(128);
    for (int c = ' '; c <= '~'; c++)
    {
        FontGlyph glyph;

        // How wide is this character.
        int ax;
        int lsb;
        stbtt_GetCodepointHMetrics(&font, c, &ax, &lsb);

        // Get bounding box for character (may be offset to account for chars that dip about or
        // below the line.
        int cX1, cY1, cX2, cY2;
        stbtt_GetCodepointBitmapBox(&font, c, scale, scale, &cX1, &cY1, &cX2, &cY2);
        lowest = std::min(lowest, cY1);

        // Compute y (different characters have different heights.
        int y = ascent + cY1;

        // Render character (stride and offset is important here).
        int byteOffset = (int)(x + roundf(lsb * scale) + (y * textureWidth));
        stbtt_MakeCodepointBitmap(
          &font, &bitmap[byteOffset], cX2 - cX1, cY2 - cY1, textureWidth, scale, scale, c);

        glyph.m_textureOffset = {(x + roundf(lsb * scale)), textureHeight - y};
        glyph.m_offset        = {roundf(lsb * scale), lineGap - (-1 * cY1)};
        // glyph.m_size          = {cX2, -1 * cY2};
        glyph.m_size    = {cX2 - cX1, -1 * (cY2 - cY1)};
        glyph.m_lead    = roundf(lsb * scale);
        glyph.m_advance = roundf(ax * scale);
        glyph.m_char    = c;
        m_atlas.emplace_back(glyph);
        // BR_TRACE(
        //  "{}: offset ({}, {}), size ({}, {}), lead {}, advance {}, c1 ({}, {}), c2 ({}, {}), pos
        //  "
        //  "({}, {})",
        //  (char)glyph.m_char,
        //  glyph.m_offset.x,
        //  glyph.m_offset.y,
        //  glyph.m_size.x,
        //  glyph.m_size.y,
        //  glyph.m_lead,
        //  glyph.m_advance,
        //  cX1,
        //  cY1,
        //  cX2,
        //  cY2,
        //  x,
        //  y);

        // Advance x.
        x += (int)roundf(ax * scale);

        // Add kerning.
        int kern;
        kern = stbtt_GetCodepointKernAdvance(&font, c, c + 1);
        x += (int)roundf(kern * scale);
    }

    uint32_t* textureData = new uint32_t[(size_t)textureWidth * textureHeight];

    // for (size_t i = 0; i < (size_t)textureWidth * textureHeight; i++)
    //{
    //    uint32_t r     = (uint32_t)bitmap[i] << 24;
    //    uint32_t g     = (uint32_t)bitmap[i] << 16;
    //    uint32_t b     = (uint32_t)bitmap[i] << 8;
    //    uint32_t a     = (uint32_t)bitmap[i];
    //    textureData[i] = r | g | b | a;
    //}

    for (size_t y = 0; y < textureHeight; y++)
    {
        size_t ry = textureHeight - y;
        for (size_t x = 0; x < textureWidth; x++)
        {
            size_t i  = (y * textureWidth + x);
            size_t ri = ((ry - 1) * textureWidth + x);
            // Basically duplicate the alpha channel on each colors.
            uint32_t r     = (uint32_t)bitmap[ri] << 24;
            uint32_t g     = (uint32_t)bitmap[ri] << 16;
            uint32_t b     = (uint32_t)bitmap[ri] << 8;
            uint32_t a     = (uint32_t)bitmap[ri];
            textureData[i] = r | g | b | a;
        }
    }

    m_fontMap = Texture2D::Create(textureWidth, textureHeight, 4);
    m_fontMap->SetData(textureData, textureWidth * textureHeight * 4);

    for (auto& glyph : m_atlas)
    {
        glyph.m_texture = SubTexture2D::CreateFromCoords(
          m_fontMap, glyph.m_textureOffset, {1.0f, 1.0f}, glyph.m_size);
    }

    delete[] textureData;
    delete[] bitmap;
    delete[] fontFile;

#if 0
    // Generate all the bit maps we want (from ASCII 0 to ASCII 256):
    std::array<PreTextureGlyph, 256> glyphs;
    for (size_t c = 0; c < 256; c++)
    {
        int w, h;
        // Get the bitmap for the current character, scaled properly.
        uint8_t* bitmap = stbtt_GetCodepointBitmap(
          &font, 0, stbtt_ScaleForPixelHeight(&font, 250), c, &w, &h, nullptr, nullptr);

        // The bitmap generated by stbtt is 1 channel only, we want a 4 channel RGBA bitmap.
        PreTextureGlyph newGlyph;
        newGlyph.data = new uint32_t[(size_t)w * h];
        BR_CORE_ASSERT(newGlyph.data, "Unable to allocate memory for font bitmap!");
        newGlyph.w = w;
        newGlyph.h = h;
        newGlyph.c = c;

        stbtt_GetCodepointHMetrics(&font, c, &newGlyph.advance, &newGlyph.leftSideBearing);

        // The bitmap generated by stbtt is also vertically flipped, we need to take care of that
        // too.
        for (size_t y = 0; y < h; y++)
        {
            size_t ry = h - y;
            for (size_t x = 0; x < w; x++)
            {
                size_t i  = (y * w + x);
                size_t ri = (ry * w + x);
                // Basically duplicate the alpha channel on each colors.
                uint32_t r       = (uint32_t)bitmap[ri] << 24;
                uint32_t g       = (uint32_t)bitmap[ri] << 16;
                uint32_t b       = (uint32_t)bitmap[ri] << 8;
                uint32_t a       = (uint32_t)bitmap[ri];
                newGlyph.data[i] = r | g | b | a;
            }
        }

        // Store that glyph.
        glyphs[c] = newGlyph;
    }

    // Sort all the glyphs from highest pixel count to lowest pixel count.
    std::sort(glyphs.begin(), glyphs.end(), [](const PreTextureGlyph& a, const PreTextureGlyph& b) {
        return a > b;
    });

    // Find the tallest glyph, that will be our vertical stride.
    size_t vStride = 0;
    for (const auto& glyph : glyphs)
    {
        if (glyph.h > vStride)
        {
            vStride = glyph.h;
        }
    }

    // We can now calculate how much space we need for our texture.
    // Every character have the same height to simplify the algorithm.
    size_t pixelCnt = 0;
    for (const auto& glyph : glyphs)
    {
        pixelCnt += vStride * glyph.w;
    }

    // Map all glyphs into this new texture.
    // We need to keep track of the width and height of this texture too.
    size_t fontMapWidth       = 4096;    // Font map will have 1:1 aspect ratio.
    size_t fontMapHeight      = 0;
    size_t currentWidthOffset = 0;

    // Add extra buffer space.
    pixelCnt += vStride * fontMapWidth;

    BR_CORE_TRACE("vStride: {}, pixelCnt: {}", vStride, pixelCnt);
    // Buffer that will contain a map of all glyphs.
    uint32_t* fontTextureData = new uint32_t[pixelCnt];
    BR_CORE_ASSERT(fontTextureData, "Unable to allocate memory for font texture data!");


    for (auto& glyph : glyphs)
    {
        // If this glyph can't fit on this line:
        if (currentWidthOffset + glyph.w >= fontMapWidth)
        {
            // Move to the next line.
            fontMapHeight += vStride;
            currentWidthOffset = 0;
        }
        BR_CORE_ASSERT(fontMapHeight * fontMapWidth < pixelCnt,
                       "Unable to pack glyphs together! h: {}, w: {}, cnt: {}",
                       fontMapHeight,
                       fontMapWidth,
                       pixelCnt);

        // Save offset into glyph.
        glyph.xoff = currentWidthOffset;
        glyph.yoff = fontMapHeight;

        // Copy glyph data into map.
        for (size_t y = 0; y < glyph.h; y++)
        {
            size_t mapIndex = ((fontMapHeight + y) * fontMapWidth + currentWidthOffset);
            for (size_t x = 0; x < glyph.w; x++)
            {
                size_t glyphIndex             = (y * glyph.w + x);
                fontTextureData[mapIndex + x] = glyph.data[glyphIndex];
            }
        }

        currentWidthOffset += glyph.w;
    }

    // Create our font map texture.
    m_fontMap = Texture2D::Create(fontMapWidth, fontMapHeight, 4);
    m_fontMap->SetData((void*)fontTextureData, fontMapWidth * fontMapHeight * 4);

    delete[] fontTextureData;

    // Create a sub-texture for each glyphs.
    m_atlas.resize(256);
    for (auto& glyph : glyphs)
    {
        // Free up glyph's buffer since we don't need it anymore.
        delete[] glyph.data;

        Ref<SubTexture2D> glyphTexture =
          SubTexture2D::CreateFromCoords(m_fontMap, {glyph.xoff, glyph.yoff}, {glyph.w, glyph.h});
        FontGlyph fontGlyph =
          FontGlyph(glyphTexture, {glyph.w, glyph.h}, {glyph.xoff, glyph.yoff}, (float)glyph.w);
        m_atlas[glyph.c] = fontGlyph;
    }
#endif
}


const FontGlyph& OpenGLFontAtlas::GetCharacterTexture(char c) const
{
    // Make sure the requested character is valid.
    // BR_CORE_ASSERT((c >= 32 || c <= 126), "Invalid character '{}' requested", c);
    // return m_atlas[(size_t)c];
    for (const auto& glyph : m_atlas)
    {
        if (glyph.m_char == c) { return glyph; }
    }

    // Unable to find char, return '?'.
    return GetCharacterTexture('?');
}

}    // namespace Brigerad
