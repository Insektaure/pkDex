#pragma once

#include <string>

// Function to check if a new version is available
// Returns:
//   1 if a new version is available (newVersion will contain the new version)
//   0 if the current version is the latest
//   -1 if there was a network error or other failure
int checkForNewVersion(std::string& newVersion);

// Function to check for updates and notify the user if a new version is available
// Used for automatic check at app launch
void checkForUpdatesAndNotify();

// Function to manually check for updates and notify the user of the result
// Used when the user clicks the "Check for updates" button
// Returns true if the check was successful, false if WiFi is not connected
bool manualCheckForUpdates();

// Function to download the latest version of the application
// Returns true if the download was successful, false otherwise
bool downloadLatestVersion(const std::string& version);

// Helper function to start the download in a background thread
void startDownload(const std::string& version, const std::string& downloadUrl, std::string* versionCopy);

// Function to download the updater application
// Returns true if the download was successful, false otherwise
bool downloadUpdater(const std::string& version);
