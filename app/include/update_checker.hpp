#pragma once

#include <string>

// Function to check if a new version is available
// In a real implementation, this would make an HTTP request to a server
// For this example, we'll simulate a new version being available
bool checkForNewVersion(std::string& newVersion);

// Function to check for updates and notify the user if a new version is available
// Used for automatic check at app launch
void checkForUpdatesAndNotify();

// Function to manually check for updates and notify the user of the result
// Used when the user clicks the "Check for updates" button
// Returns true if the check was successful, false if WiFi is not connected
bool manualCheckForUpdates();
