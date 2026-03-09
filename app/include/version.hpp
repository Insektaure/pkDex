#pragma once

#include <string>

// Version is defined via CMake compile definitions (PKDEX_VERSION_MAJOR, etc.)
// Fallback defaults are provided for IDE support / non-CMake builds.
#ifndef PKDEX_VERSION_MAJOR
#define PKDEX_VERSION_MAJOR 1
#endif
#ifndef PKDEX_VERSION_MINOR
#define PKDEX_VERSION_MINOR 6
#endif
#ifndef PKDEX_VERSION_ALTER
#define PKDEX_VERSION_ALTER 5
#endif

#define PKDEX_STRINGIFY_(x) #x
#define PKDEX_STRINGIFY(x) PKDEX_STRINGIFY_(x)

namespace pkdex {
    // Current application version - single source of truth is CMakeLists.txt
    static const std::string CURRENT_VERSION = "v" PKDEX_STRINGIFY(PKDEX_VERSION_MAJOR) "." PKDEX_STRINGIFY(PKDEX_VERSION_MINOR) "." PKDEX_STRINGIFY(PKDEX_VERSION_ALTER);

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
