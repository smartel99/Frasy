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

#include <string>


class MyFrasyInterpreter : public Frasy::Interpreter {
public:
    MyFrasyInterpreter() : Interpreter("Frasy - Demo Mode") { pushLayer(new MyMainApplicationLayer()); }
};


Brigerad::Application* Brigerad::CreateApplication(int argc, char** argv)
{
    return new MyFrasyInterpreter();
}

/** @} */
