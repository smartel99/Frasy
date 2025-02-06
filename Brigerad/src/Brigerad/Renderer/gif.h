/**
 * @file    gif.h
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
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */


#ifndef BRIGERAD_RENDERER_GIF_H
#define BRIGERAD_RENDERER_GIF_H

#include "Brigerad/Core/Timestep.h"
#include "Texture.h"

#include <string_view>
#include <utility>

namespace Brigerad {
class Gif {
public:
    explicit Gif(std::string_view path);
    explicit Gif(Ref<Texture2D> texture) : m_texture(std::move(texture)), m_isStill(true) {}
    ~Gif();

    Ref<Texture2D> texture() const { return m_texture; }

    void onUpdate(Timestep timestep);

private:
    unsigned char* getFrameData(int frame);
    int            getFrameDelay(int frame);
    /**
     *
     * @return {frameNumber, delay}
     */
    std::pair<int, int> nextFrame();

private:
    Ref<Texture2D> m_texture;
    bool           m_isStill  = false;
    int            m_atFrame  = 0;
    int            m_frames   = 0;
    int            m_channels = 0;
    int            m_width    = 0;
    int            m_height   = 0;

    unsigned char* m_data              = nullptr;
    int*           m_delays            = nullptr;
    int            m_currentFrameDelay = 0;
};

}    // namespace Brigerad
#endif    // BRIGERAD_RENDERER_GIF_H
