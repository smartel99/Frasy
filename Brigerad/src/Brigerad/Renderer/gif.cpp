/**
 * @file    gif.cpp
 * @author  Sam Martel
 * @date    2025-02-06
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
 * not, see <https://www.gnu.org/licenses/>.
 */
#include "gif.h"

#include "asset_manager.h"
#include "Brigerad/Debug/Instrumentor.h"
#include "stb_image.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <vector>


namespace Brigerad {
Gif::Gif(std::string_view path)
{
    auto          fileSize = std::filesystem::file_size(path);
    std::ifstream file {path.data(), std::ios::binary};

    std::vector<uint8_t> data = std::vector<uint8_t>(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);

    {
        BR_PROFILE_SCOPE("stbi_load_gif_from_memory - Gif::Gif(const std::string&)");
        m_data = stbi_load_gif_from_memory(
          data.data(), static_cast<int>(data.size()), &m_delays, &m_width, &m_height, &m_frames, &m_channels, 0);
    }
    if (m_data == nullptr) {
        BR_APP_ERROR("Unable to load Gif from file '{}': {}", path, stbi_failure_reason());
        m_texture = AssetManager::GetMissingTextureTexture();
        m_isStill = true;
        return;
    }
    BR_APP_DEBUG("Loaded Gif from file '{}': {} frames, {}x{}, {} channels, duration: {}",
                 path,
                 m_frames,
                 m_width,
                 m_height,
                 m_channels,
                 std::accumulate(m_delays, m_delays + m_frames, 0));

    m_texture = Texture2D::Create(m_width, m_height, static_cast<uint8_t>(m_channels));
    m_texture->SetData(getFrameData(m_atFrame), m_width * m_height * m_channels);
    nextFrame();
}

Gif::~Gif()
{
    stbi_image_free(m_data);
    stbi_image_free(m_delays);
}

void Gif::onUpdate(Timestep timestep)
{
    m_currentFrameDelay -= static_cast<int>(timestep.GetMilliseconds());

    while (m_currentFrameDelay <= 0) {
        int remainder = m_currentFrameDelay;
        nextFrame();
        m_currentFrameDelay -= remainder;
    }
    m_texture->SetData(getFrameData(m_atFrame), m_width * m_height * m_channels);
}

unsigned char* Gif::getFrameData(int frame)
{
    return m_data + (static_cast<ptrdiff_t>(m_width * m_height * m_channels * frame));
}

int Gif::getFrameDelay(int frame)
{
    return m_delays[frame];
}

std::pair<int, int> Gif::nextFrame()
{
    m_atFrame++;
    if (m_atFrame >= m_frames) { m_atFrame = 0; }
    m_currentFrameDelay = m_delays[m_atFrame];
    return {m_atFrame, m_currentFrameDelay};
}
}    // namespace Brigerad
