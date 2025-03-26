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

#include <layers/main_application_layer.h>
#include <utils/lua/orchestrator/orchestrator.h>
#include <utils/lua/popup.h>

#include <Brigerad/Utils/dialogs/warning.h>
#include <filesystem>
#include <map>

class MyMainApplicationLayer final : public Frasy::MainApplicationLayer {
    struct ProductInfo {
        std::string                                            environmentPath;
        std::string                                            testPath;
        std::string                                            name;
        std::map<std::string, std::filesystem::file_time_type> lastModifiedTimes;
    };

public:
    MyMainApplicationLayer()                                         = default;
    MyMainApplicationLayer(const MyMainApplicationLayer&)            = delete;
    MyMainApplicationLayer& operator=(const MyMainApplicationLayer&) = delete;
    MyMainApplicationLayer(MyMainApplicationLayer&&)                 = delete;
    MyMainApplicationLayer& operator=(MyMainApplicationLayer&&)      = delete;

    ~MyMainApplicationLayer() override = default;

    void onAttach() override;

    void onUpdate(Brigerad::Timestep ts) override;

protected:
    void renderControlRoom() override;

private:
    bool renderEnvironmentError();
    bool renderProductDropdownMenu();
    void renderReloadAndRefreshLuaButton();
    void renderOperatorField();
    void renderSerialField(std::size_t uut);
    void renderRunButton();
    void renderUutIcon(std::size_t uut);
    void handleTestRepeat();

    void doTests();

    void makeOrchestrator(const std::string& name, const std::string& envPath, const std::string& testPath);

    void loadLuaFunctions(sol::state_view lua);
    void loadProducts();
    bool shouldRegenerate();

    static std::vector<ProductInfo> detectProducts();

    static std::map<std::string, std::filesystem::file_time_type> getProductFileModificationTimes(
      const std::string& path);

    ProductInfo& getActiveProduct()
    {
        auto it = std::ranges::find_if(
          m_products, [active = m_activeProduct](const ProductInfo& product) { return product.name == active; });
        BR_ASSERT(it != m_products.end(), "Active product ('{}') not found!", m_activeProduct);
        return *it;
    }

private:
    static constexpr std::size_t                        s_operatorNameMaxLength = 20;
    std::array<char, s_operatorNameMaxLength>           m_operatorField {};
    static constexpr std::size_t                        s_serialNumberLength = 32;
    std::vector<std::array<char, s_serialNumberLength>> m_serialsFields {};
    std::vector<std::string>                            m_serials {};

    bool                     m_skipVerification = false;
    std::string              m_activeProduct;
    std::vector<ProductInfo> m_products;

    bool m_testJustFinished = false;
    int  m_repeatCount      = 0;

    const ImVec2          m_buttonSize       = ImVec2 {100.0f, 100.0f};
    static constexpr auto s_lineWidth        = 400;
    static constexpr auto s_labelWidth       = 150;
    static constexpr auto s_inputWidth       = s_lineWidth - s_labelWidth;
    ImGuiWindowFlags      m_imGuiWindowFlags = ImGuiWindowFlags_NoTitleBar |    //
                                          ImGuiWindowFlags_NoResize |           //
                                          ImGuiWindowFlags_NoMove |             //
                                          ImGuiWindowFlags_NoNavFocus |         //
                                          ImGuiWindowFlags_NoCollapse;
};

#endif    // GUARD_MY_MAIN_APPLICATION_LAYER_H
