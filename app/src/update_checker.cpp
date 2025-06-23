#include "update_checker.hpp"
#include "version.hpp"
#include <borealis.hpp>

bool checkForNewVersion(std::string& newVersion) {
    // In a real implementation, this would fetch the latest version from a server
    // For this example, we'll simulate a new version being available
    newVersion = "v1.1.0";

    // Compare with current version using the centralized version definition
    if (pkdex::isNewerVersion(pkdex::CURRENT_VERSION, newVersion)) {
        return true; // New version available
    }

    return false; // No new version available
}

void checkForUpdatesAndNotify() {
    // Check if WiFi is connected
    if (brls::Application::getPlatform()->hasWirelessConnection()) {
        // Check for new version
        std::string newVersion;
        if (checkForNewVersion(newVersion)) {
            brls::Application::notify("New version available: " + newVersion + " (Current: " + pkdex::CURRENT_VERSION + ")");
        }
    }
}

bool manualCheckForUpdates() {
    // Check if WiFi is connected
    if (!brls::Application::getPlatform()->hasWirelessConnection()) {
        brls::Application::notify("WiFi is not connected. Please connect to WiFi to check for updates.");
        return false;
    }

    // Check for new version
    std::string newVersion;
    if (checkForNewVersion(newVersion)) {
        brls::Application::notify("New version available: " + newVersion + " (Current: " + pkdex::CURRENT_VERSION + ")");
    } else {
        brls::Application::notify("You are using the latest version: " + pkdex::CURRENT_VERSION);
    }

    return true;
}
