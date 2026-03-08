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

struct CaptureStates {
    bool normal = false;
    bool shiny = false;
    bool alpha = false;
    bool shinyAlpha = false;
};

class PokemonTracker {
public:

    /**
     * @brief Get the capture states of a Pokémon
     * 
     * @param region The region of the Pokémon
     * @param regionalDexNumber The regional dex number of the Pokémon
     * @return CaptureStates The capture states
     */
    static CaptureStates getCaptureStates(const std::string& region, const std::string& regionalDexNumber);

    /**
     * @brief Toggle a specific capture state of a Pokémon
     * 
     * @param region The region of the Pokémon
     * @param regionalDexNumber The regional dex number of the Pokémon
     * @param stateIndex 0=normal, 1=shiny, 2=alpha, 3=shinyAlpha
     * @return CaptureStates The new capture states
     */
    static CaptureStates toggleCaptureState(const std::string& region, const std::string& regionalDexNumber, int stateIndex);

    /**
     * @brief Set a specific capture state for multiple Pokémon at once
     *
     * @param region The region of the Pokémon
     * @param regionalDexNumbers The regional dex numbers of the Pokémon to update
     * @param stateIndex 0=normal, 1=shiny, 2=alpha, 3=shinyAlpha
     * @param value The value to set (true=caught, false=not caught)
     */
    static void bulkSetCaptureState(const std::string& region, const std::vector<std::string>& regionalDexNumbers, int stateIndex, bool value);

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
