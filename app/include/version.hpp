#pragma once

#include <string>

// Centralized version definition
// IMPORTANT: Keep this version in sync with the version in CMakeLists.txt
// When updating the version, update both this file and CMakeLists.txt (lines 27-30)
namespace pkdex {
    // Current application version - must match VERSION_MAJOR, VERSION_MINOR, VERSION_ALTER in CMakeLists.txt
    static const std::string CURRENT_VERSION = "v1.6.5";

    // Function to get the current version without the 'v' prefix
    inline std::string getCurrentVersionWithoutPrefix() {
        if (!CURRENT_VERSION.empty() && CURRENT_VERSION[0] == 'v') {
            return CURRENT_VERSION.substr(1);
        }
        return CURRENT_VERSION;
    }

    // Function to compare version strings (returns true if newVersion is newer)
    inline bool isNewerVersion(const std::string& currentVersion, const std::string& newVersion) {
        // Remove 'v' prefix if present
        std::string current = currentVersion;
        std::string newer = newVersion;

        if (!current.empty() && current[0] == 'v') {
            current = current.substr(1);
        }

        if (!newer.empty() && newer[0] == 'v') {
            newer = newer.substr(1);
        }

        // Parse version components
        int currentMajor = 0, currentMinor = 0, currentPatch = 0;
        int newMajor = 0, newMinor = 0, newPatch = 0;

        sscanf(current.c_str(), "%d.%d.%d", &currentMajor, &currentMinor, &currentPatch);
        sscanf(newer.c_str(), "%d.%d.%d", &newMajor, &newMinor, &newPatch);

        // Compare versions
        if (newMajor > currentMajor) return true;
        if (newMajor < currentMajor) return false;

        if (newMinor > currentMinor) return true;
        if (newMinor < currentMinor) return false;

        return newPatch > currentPatch;
    }
}
