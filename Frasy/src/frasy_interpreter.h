/**
 ******************************************************************************
 * @addtogroup frasy_interpreter
 * @{
 * @file    frasy_interpreter.h
 * @author  Samuel Martel
 * @brief   Header for the frasy_interpreter module.
 *
 * @date 12/1/2022 10:59:10 AM
 *
 ******************************************************************************
 */
#ifndef frasy_interpreter_H
#    define frasy_interpreter_H

/*****************************************************************************/
/* Includes */

#    include "Brigerad.h"
#    include "utils/internal_config.h"

#    include <memory>
#    include <string>
#    include <thread>

#    include <WinSock2.h>

/*****************************************************************************/
/* Exported Defines and Macros */


/*****************************************************************************/
/* Exported Variables */


/*****************************************************************************/
/* Exported Enums */


/*****************************************************************************/
/* Exported Structs and Classes */
class FrasyInterpreter : public Brigerad::Application
{
public:
    FrasyInterpreter(const std::string& name, const std::string& cfgPath = "config.json")
    : Application(name)
    {
        if (s_instance != nullptr)
        {
            throw std::exception("Interpreter instance already created!");
        }

        s_instance       = this;
        m_internalConfig = InternalConfig::Load(cfgPath);
    }

    ~FrasyInterpreter() override
    {
        m_internalConfig.Save();

        s_instance = nullptr;
    }

    static FrasyInterpreter& Get() { return *s_instance; }

    virtual InternalConfig& GetConfig() { return m_internalConfig; }

protected:
    inline static FrasyInterpreter* s_instance = nullptr;

    InternalConfig m_internalConfig;
};


/* Have a wonderful day :) */
#endif /* frasy_interpreter_H */
/**
 * @}
 */
/****** END OF FILE ******/
