#pragma once

#include <string>
#include <map>
#include <fstream>
#include <filesystem>

namespace pkdex {

/**
 * @brief Utility class for handling configuration settings
 */
class Config {
public:
    /**
     * @brief Get a boolean value from the config file
     * 
     * @param key The key to look for
     * @param defaultValue The default value to return if the key is not found
     * @return bool The value associated with the key, or the default value if not found
     */
    static bool getBool(const std::string& key, bool defaultValue = false);

    /**
     * @brief Set a boolean value in the config file
     * 
     * @param key The key to set
     * @param value The value to set
     * @return bool True if the operation was successful, false otherwise
     */
    static bool setBool(const std::string& key, bool value);

    /**
     * @brief Get an integer value from the config file
     * 
     * @param key The key to look for
     * @param defaultValue The default value to return if the key is not found
     * @return int The value associated with the key, or the default value if not found
     */
    static int getInt(const std::string& key, int defaultValue = 0);

    /**
     * @brief Set an integer value in the config file
     * 
     * @param key The key to set
     * @param value The value to set
     * @return bool True if the operation was successful, false otherwise
     */
    static bool setInt(const std::string& key, int value);

    /**
     * @brief Get a string value from the config file
     *
     * @param key The key to look for
     * @param defaultValue The default value to return if the key is not found
     * @return std::string The value associated with the key, or the default value if not found
     */
    static std::string getString(const std::string& key, const std::string& defaultValue = "");

    /**
     * @brief Set a string value in the config file
     *
     * @param key The key to set
     * @param value The value to set
     * @return bool True if the operation was successful, false otherwise
     */
    static bool setString(const std::string& key, const std::string& value);

private:
    /**
     * @brief Get the path to the config file
     * 
     * @return std::string The path to the config file
     */
    static std::string getConfigFilePath();

    /**
     * @brief Read the config file into a map
     * 
     * @return std::map<std::string, std::string> A map of key-value pairs from the config file
     */
    static std::map<std::string, std::string> readConfigFile();

    /**
     * @brief Write a map of key-value pairs to the config file
     * 
     * @param config The map of key-value pairs to write
     * @return bool True if the operation was successful, false otherwise
     */
    static bool writeConfigFile(const std::map<std::string, std::string>& config);

    /**
     * @brief Update a specific key in the config file without rewriting the entire file
     * 
     * @param key The key to update
     * @param value The new value for the key
     * @return bool True if the operation was successful, false otherwise
     */
    static bool updateConfigKey(const std::string& key, const std::string& value);
};

} // namespace pkdex
