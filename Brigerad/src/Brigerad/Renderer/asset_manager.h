/**
 * @file    asset_manager.h
 * @author  Samuel Martel
 * @date    2024-10-28
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
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */


#ifndef BRIGERAD_RENDERER_ASSET_MANAGER_H
#define BRIGERAD_RENDERER_ASSET_MANAGER_H

#include "../Core/File.h"
#include "../Core/Log.h"
#include "gif.h"
#include "Texture.h"

#include <string_view>
#include <unordered_map>

namespace Brigerad {
class AssetManager {
public:
    static void Init()
    {
        m_missingTextureTexture     = Texture2D::Create(1, 1);
        uint32_t magentaTextureData = 0xFFFF00FF;
        m_missingTextureTexture->SetData(&magentaTextureData, sizeof(magentaTextureData));
    }

    static Ref<Texture2D> GetMissingTextureTexture() { return m_missingTextureTexture; }

    static Ref<Texture2D> AddTexture2D(std::string_view name, std::string_view path)
    {
        auto it = m_textures2d.find(name);
        if (it != m_textures2d.end()) { return it->second; }
        if (!File::CheckIfPathExists(path)) {
            BR_CORE_ERROR("Unable to open '{}' to create Texture2D '{}'!", path, name);
            m_textures2d[name] = m_missingTextureTexture;
        }
        else {
            m_textures2d[name] = Texture2D::Create(path);
        }
        return m_textures2d[name];
    }
    static Ref<Texture2D> AddTexture2D(std::string_view path) { return AddTexture2D(path, path); }
    static Ref<Texture2D> AddTexture2D(std::string_view name, uint32_t width, uint32_t height, uint32_t channels = 4)
    {
        m_textures2d[name] = Texture2D::Create(width, height, channels);
        return m_textures2d[name];
    }

    static Ref<Texture2D> GetTexture2D(std::string_view name) { return m_textures2d.at(name); }

    static Ref<Gif> AddGif(std::string_view name, std::string_view path)
    {
        auto it = m_gifs.find(name);
        if (it != m_gifs.end()) { return it->second; }
        if (!File::CheckIfPathExists(path)) {
            BR_CORE_ERROR("Unable to open '{}' to create Gif '{}'!", path, name);
            m_gifs[name] = std::make_shared<Gif>(m_missingTextureTexture);
        }
        else {
            m_gifs[name] = std::make_shared<Gif>(path);
        }
        return m_gifs[name];
    }
    static Ref<Gif> AddGif(std::string_view path) { return AddGif(path, path); }

    static Ref<Gif> GetGif(std::string_view name) { return m_gifs.at(name); }

private:
    friend class Application;
    inline static Ref<Texture2D>                                       m_missingTextureTexture;
    inline static std::unordered_map<std::string_view, Ref<Texture2D>> m_textures2d;
    inline static std::unordered_map<std::string_view, Ref<Gif>>       m_gifs;
};
}    // namespace Brigerad

#endif    // BRIGERAD_RENDERER_ASSET_MANAGER_H
