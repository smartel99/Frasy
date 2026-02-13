/**
 * @file    test_viewer.h
 * @author  Paul Thomas
 * @date    5/2/2023
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
#ifndef KONGSBERG_FRASY_FRASY_SRC_LAYERS_TEST_VIEWER_H
#define KONGSBERG_FRASY_FRASY_SRC_LAYERS_TEST_VIEWER_H
#include "Brigerad/Core/Layer.h"
#include "utils/lua/orchestrator/orchestrator.h"
#include "utils/models/sequence.h"

#include <string>
#include <unordered_map>

namespace Frasy
{
class TestViewer : public Brigerad::Layer
{
public:
    class Interface;

private:
    enum class ListStatus;

public:
    TestViewer();
    ~TestViewer() override = default;

    void onImGuiRender() override;

    void SetVisibility(bool visibility) { m_isVisible = visibility; }
    void SetInterface(Interface* interface) { m_interface = interface; }

private:
    void RenderSequence(const std::string& name, const Models::Sequence& sequence);

private:
    bool                                        m_isVisible = false;
    std::unordered_map<std::string, ListStatus> m_listStatus;
    Interface*                                  m_interface;
};

class TestViewer::Interface
{
public:
    static Interface* GetDefault();
    virtual ~Interface() = default;
    virtual const Models::Solution& GetSolution();
    virtual void                    generate();
    virtual void                    setSequenceEnable(const std::string& sequence, bool enable);
    virtual void                    setTestEnable(const std::string& sequence, const std::string& test, bool enable);
};

enum class TestViewer::ListStatus
{
    unknown,
    expanded,
    folded,
};

}    // namespace Frasy


#endif    // KONGSBERG_FRASY_FRASY_SRC_LAYERS_TEST_VIEWER_H
