#pragma once

#include <string>
#include <map>
#include <fstream>
#include <filesystem>
#include <vector>

namespace pkdex {

/**
 * @brief Utility class for tracking captured Pokémon
 */
class PokemonTracker {
public:
    /**
     * @brief Check if a Pokémon is captured
     * 
     * @param region The region of the Pokémon
     * @param regionalDexNumber The regional dex number of the Pokémon
     * @return bool True if the Pokémon is captured, false otherwise
     */
    static bool isCaptured(const std::string& region, const std::string& regionalDexNumber);

    /**
     * @brief Toggle the capture status of a Pokémon
     * 
     * @param region The region of the Pokémon
     * @param regionalDexNumber The regional dex number of the Pokémon
     * @return bool The new capture status (true if captured, false if not)
     */
    static bool toggleCaptureStatus(const std::string& region, const std::string& regionalDexNumber);

    /**
     * @brief Reset all Pokémon capture statuses for all regions
     * 
     * @return bool True if the operation was successful, false otherwise
     */
    static bool resetAllCaptureStatus();

    /**
     * @brief Reset all Pokémon capture statuses for a specific region
     * 
     * @param region The region to reset capture statuses for
     * @return bool True if the operation was successful, false otherwise
     */
    static bool resetRegionCaptureStatus(const std::string& region);

    /**
     * @brief Get all available regions
     * 
     * @return std::vector<std::string> A vector of all available regions
     */
    static std::vector<std::string> getAllRegions();

private:
    /**
     * @brief Get the path to the tracker file for a specific region
     * 
     * @param region The region to get the tracker file for
     * @return std::string The path to the tracker file
     */
    static std::string getTrackerFilePath(const std::string& region);

    /**
     * @brief Read the tracker file for a specific region into a map
     * 
     * @param region The region to read the tracker file for
     * @return std::map<std::string, std::string> A map of key-value pairs from the tracker file
     */
    static std::map<std::string, std::string> readTrackerFile(const std::string& region);

    /**
     * @brief Write a map of key-value pairs to the tracker file for a specific region
     * 
     * @param region The region to write the tracker file for
     * @param tracker The map of key-value pairs to write
     * @return bool True if the operation was successful, false otherwise
     */
    static bool writeTrackerFile(const std::string& region, const std::map<std::string, std::string>& tracker);

    /**
     * @brief Update a specific key in the tracker file for a specific region without rewriting the entire file
     * 
     * @param region The region to update the tracker file for
     * @param key The key to update
     * @param value The new value for the key
     * @return bool True if the operation was successful, false otherwise
     */
    static bool updateTrackerKey(const std::string& region, const std::string& key, const std::string& value);

    /**
     * @brief Generate a key for the tracker file based on region and regional dex number
     * 
     * @param region The region of the Pokémon
     * @param regionalDexNumber The regional dex number of the Pokémon
     * @return std::string The key for the tracker file
     */
    static std::string generateKey(const std::string& region, const std::string& regionalDexNumber);
};

} // namespace pkdex
