#pragma once

#include <string>
#include <functional>

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

// Generic download function that can be used by all download functions
// Parameters:
//   downloadUrl: URL to download from
//   outputPath: Path where the downloaded file will be saved
//   progressLabel: Label to display in progress notifications (e.g., version number or "High-Res Pack")
//   onComplete: Callback function to execute when download completes successfully
// Returns true if the download was started successfully, false otherwise
bool downloadFile(const std::string& downloadUrl, const std::string& outputPath, 
                 const std::string& progressLabel, std::function<void()> onComplete = nullptr);

// Function to download the updater application
// Returns true if the download was successful, false otherwise
bool downloadUpdater(const std::string& version);

// Function to download the high-resolution image pack to the root of the SD card
// Returns true if the download was successful, false otherwise
bool downloadHighResImagePack();

// Function to extract the high-resolution image pack from the zip file
// Returns true if the extraction was successful, false otherwise
bool extractHighResImagePack();

// Function to extract a zip file with progress reporting
// progressCallback is called with a value between 0.0 and 1.0 to indicate progress
// Returns true if the extraction was successful, false otherwise
bool extractZipFile(const std::string& zipFilePath, const std::string& extractPath, 
                   std::function<void(float)> progressCallback = nullptr);
