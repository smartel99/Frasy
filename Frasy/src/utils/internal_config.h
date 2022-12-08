/**
 ******************************************************************************
 * @addtogroup Config
 * @{
 * @file    Config
 * @author  Samuel Martel
 * @brief   Header for the Config module.
 *
 * @note https://github.com/nlohmann/json
 *
 * @date 1/4/2020 1:41:45 PM
 *
 ******************************************************************************
 */
#ifndef _Config
#    define _Config

/*****************************************************************************/
/* Includes */
#    include "../vendor/json.hpp"

#    include <string_view>

class InternalConfig
{
public:
    using config_t    = nlohmann::json;
    using exception_t = config_t::exception;

public:
    InternalConfig() noexcept                                 = default;
    InternalConfig(const InternalConfig&) noexcept            = default;
    InternalConfig(InternalConfig&&) noexcept                 = default;
    InternalConfig& operator=(const InternalConfig&) noexcept = default;
    InternalConfig& operator=(InternalConfig&&) noexcept      = default;

    explicit InternalConfig(const std::string& path);

    InternalConfig(const std::string& key, const config_t& config) : m_path(key), m_config(config)
    {
    }

    static InternalConfig Load(const std::string& path);
    static void           Save(const std::string& path, const InternalConfig& cfg);
    void                  Save() const { Save(m_path, *this); }

    template<class T>
    void SetField(const std::string& key, const T& val)
    {
        m_config[key] = val;
    }

    template<>
    void SetField(const std::string& key, const InternalConfig& val)
    {
        m_config[key] = val.m_config;
    }

    template<class T = InternalConfig>
    T GetField(const std::string& key, const T& def = {}) const
    {
        try
        {
            if constexpr(std::is_same_v<T, InternalConfig>)
            {
                return {key, m_config.at(key)};
            }
            else { return static_cast<T>(m_config.at(key)); }
        }
        catch (exception_t&)
        {
            // Requested item doesn't exist.
            // throw std::invalid_argument("Field Not Found!");
            return def;
        }
    }

    template<typename T = int>
    T GetFieldFromObject(const config_t& obj, const std::string& field)
    {
        try
        {
            return obj[field];
        }
        catch (const exception_t&)
        {
            return T();
        }
    }

private:
    std::string m_path;
    config_t    m_config = {};
};


/* Have a wonderful day :) */
#endif /* _Config */
/**
 * @}
 */
/****** END OF FILE ******/
