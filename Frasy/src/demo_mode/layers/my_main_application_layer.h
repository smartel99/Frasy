/**
 * @file    my_main_application_layer.h
 * @author  Samuel Martel
 * @date    2022-12-05
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

#ifndef GUARD_MY_MAIN_APPLICATION_LAYER_H
#define GUARD_MY_MAIN_APPLICATION_LAYER_H

#include "../../layers/main_application_layer.h"

class MyMainApplicationLayer final : public Frasy::MainApplicationLayer
{
public:
    MyMainApplicationLayer();
    ~MyMainApplicationLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;

protected:
    void RenderControlRoom() override;

private:
};

#endif    // GUARD_MY_MAIN_APPLICATION_LAYER_H