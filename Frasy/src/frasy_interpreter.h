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
#    include "utils/communication/serial/device_map.h"
#    include "utils/config.h"

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
namespace Frasy
{
class FrasyInterpreter : public Brigerad::Application
{
public:
    FrasyInterpreter(const std::string& name, const std::string& cfgPath = "config.json") : Application(name)
    {
        if (s_instance != nullptr) { throw std::exception("Interpreter instance already created!"); }

        s_instance                               = this;
        m_internalConfig                         = Frasy::Config(cfgPath);
        static constexpr const char* s_usrCfgKey = "UserConfigPath";
        std::string                  userCfgPath = m_internalConfig.getField<std::string>(s_usrCfgKey);
        if (!userCfgPath.empty()) { m_userConfig = Frasy::Config(userCfgPath); }
        else { BR_CORE_ERROR("'{}' not found in {}!", s_usrCfgKey, cfgPath); }
    }

    ~FrasyInterpreter() override
    {
        m_internalConfig.save();
        m_userConfig.save();

        s_instance = nullptr;
    }

    static FrasyInterpreter& Get() { return *s_instance; }

    virtual Config&                   getConfig() { return m_internalConfig; }
    virtual Config&                   GetUserConfig() { return m_userConfig; }
    virtual Serial::DeviceMap& GetDevices() { return m_deviceMap; }


protected:
    inline static FrasyInterpreter* s_instance = nullptr;

    Config m_internalConfig;
    Config m_userConfig;

    Serial::DeviceMap m_deviceMap;
};
}    // namespace Frasy


/* Have a wonderful day :) */
#endif /* frasy_interpreter_H */
/**
 * @}
 */
/****** END OF FILE ******/
