﻿/**
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

#include "Brigerad.h"
#include "Brigerad/Core/EntryPoint.h"
#include "frasy_interpreter.h"
#include "layers/my_main_application_layer.h"

#include <string>


class MyFrasyInterpreter : public Frasy::FrasyInterpreter
{
public:
    MyFrasyInterpreter() : FrasyInterpreter("Frasy - Demo Mode") { PushLayer(new MyMainApplicationLayer()); }
};


Brigerad::Application* Brigerad::CreateApplication()
{
    return new MyFrasyInterpreter();
}

/** @} */
