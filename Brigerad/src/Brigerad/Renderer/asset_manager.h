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

    static Ref<Texture2D> AddTexture2D(std::string_view name, std::string_view path)
    {
        if (!File::CheckIfPathExists(path)) {
            BR_CORE_ERROR("Unable to open '{}' to create Texture2D '{}'!", path, name);
            m_textures2d[name] = m_missingTextureTexture;
        }
        else {
            m_textures2d[name] = Texture2D::Create(path);
        }
        return m_textures2d[name];
    }
    static Ref<Texture2D> GetTexture2D(std::string_view name) { return m_textures2d.at(name); }

private:
    inline static Ref<Texture2D>                                       m_missingTextureTexture;
    inline static std::unordered_map<std::string_view, Ref<Texture2D>> m_textures2d;
};
}    // namespace Brigerad

#endif    // BRIGERAD_RENDERER_ASSET_MANAGER_H
