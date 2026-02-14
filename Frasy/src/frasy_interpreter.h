/**
 * @file    frasy_interpreter.h
 * @author  Samuel Martel
 * @brief   Header for the frasy_interpreter module.
 *
 * @date 12/1/2022 10:59:10 AM
 */
#ifndef FRASY_INTERPRETER_H
#define FRASY_INTERPRETER_H

#include "Brigerad.h"
#include "utils/communication/serial/device_map.h"

#include <json.hpp>
#include <string>

namespace Frasy
{
    class Interpreter : public Brigerad::Application
    {
    public:
        explicit Interpreter(const std::string& name) : Application(name), m_config(loadConfig())
        {
            if (s_instance != nullptr) { throw std::runtime_error("Interpreter instance already created!"); }
            s_instance = this;
        }

        ~Interpreter() override
        {
            saveConfig();
            s_instance = nullptr;
        }

        static Interpreter& Get() { return *s_instance; }

        Serial::DeviceMap& GetDevices() { return m_deviceMap; }

        nlohmann::json& getConfig() { return m_config; }
        const nlohmann::json& getConfig() const { return m_config; }

        void saveConfig() const;

    protected:
        inline static Interpreter* s_instance = nullptr;


        nlohmann::json m_config;
        Serial::DeviceMap m_deviceMap;

    private:
        static nlohmann::json loadConfig();
    };
} // namespace Frasy


/* Have a wonderful day :) */
#endif /* frasy_interpreter_H */
