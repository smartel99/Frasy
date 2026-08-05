/**
 ******************************************************************************
 * @addtogroup Frasy
 * @{
 * @file    frasy_interpreter.cpp
 * @author  Samuel Martel
 * @brief
 *
 * @date 2020/09/17 3:23:33 PM
 *
 ******************************************************************************
 */

#include "layers/my_main_application_layer.h"
#include <Brigerad.h>
#include <Brigerad/Core/EntryPoint.h>
#include <frasy_interpreter.h>
#include <utils/headless/product_provider.h>

#include <string>


class DemoProductProvider : public Frasy::Headless::ProductProvider {
public:
    bool validateSerialNumber(const std::string& serial) override
    {
        return !serial.empty();    // Demo: any non-empty serial is valid
    }

    bool setup(Frasy::Lua::Orchestrator& orchestrator,
               Frasy::CanOpen::CanOpen&   canOpen,
               const std::string&         product,
               const std::string&         envPath,
               const std::string&         testsDir) override
    {
        if (!orchestrator.loadUserFiles(envPath, testsDir)) { return false; }

        canOpen.stop();
        canOpen.clearNodes();
        const auto& [ibs, uuts, teams] = orchestrator.getMap();
        for (const auto& ib : ibs | std::views::values) {
            canOpen.addNode(ib.nodeId, ib.name, ib.edsPath);
        }
        canOpen.start();
        return true;
    }
};


class MyFrasyInterpreter : public Frasy::Interpreter {
public:
    MyFrasyInterpreter() : Interpreter("Frasy - Demo Mode")
    {
        setProductProvider(std::make_unique<DemoProductProvider>());
        pushLayer(new MyMainApplicationLayer());
    }
};


Brigerad::Application* Brigerad::CreateApplication(int argc, char** argv)
{
    return new MyFrasyInterpreter();
}

/** @} */
