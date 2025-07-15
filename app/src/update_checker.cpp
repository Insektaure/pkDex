#include "update_checker.hpp"
#include "version.hpp"
#include <borealis.hpp>
#include <switch.h>
#include <string>
#include <cstring>
#include <curl/curl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <minizip/unzip.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

// Function to create directory recursively
bool createDirRecursively(const std::string& path) {
    brls::Logger::debug("Creating directory: {}", path);

    // Skip if the directory already exists
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            brls::Logger::debug("Directory already exists: {}", path);
            return true;
        } else {
            brls::Logger::error("Path exists but is not a directory: {}", path);
            return false;
        }
    }

    // Create parent directories
    size_t pos = path.find_last_of('/');
    if (pos != std::string::npos) {
        std::string parentPath = path.substr(0, pos);
        if (!parentPath.empty()) {
            brls::Logger::debug("Creating parent directory: {}", parentPath);
            if (!createDirRecursively(parentPath)) {
                brls::Logger::error("Failed to create parent directory: {}", parentPath);
                return false;
            }
        }
    }

    // Create the directory
    brls::Logger::debug("Creating directory with mkdir: {}", path);
    if (mkdir(path.c_str(), 0777) != 0 && errno != EEXIST) {
        brls::Logger::error("Failed to create directory: {}, error: {}", path, strerror(errno));
        return false;
    }

    brls::Logger::debug("Successfully created directory: {}", path);
    return true;
}

// Function to extract a zip file with progress reporting
bool extractZipFile(const std::string& zipFilePath, const std::string& extractPath, 
                   std::function<void(float)> progressCallback) {
    brls::Logger::debug("Attempting to extract zip file: {} to {}", zipFilePath, extractPath);

    // Check if the zip file exists
    struct stat zipStat;
    if (stat(zipFilePath.c_str(), &zipStat) != 0) {
        brls::Logger::error("Zip file does not exist: {}, error: {}", zipFilePath, strerror(errno));
        return false;
    }

    brls::Logger::debug("Zip file exists, size: {} bytes", zipStat.st_size);

    unzFile zipFile = unzOpen(zipFilePath.c_str());
    if (!zipFile) {
        brls::Logger::error("Failed to open zip file: {}, error: {}", zipFilePath, strerror(errno));
        return false;
    }

    brls::Logger::debug("Successfully opened zip file: {}", zipFilePath);

    // Create the extraction directory
    brls::Logger::debug("Creating extraction directory: {}", extractPath);
    if (!createDirRecursively(extractPath)) {
        brls::Logger::error("Failed to create extraction directory: {}", extractPath);
        unzClose(zipFile);
        return false;
    }
    brls::Logger::debug("Extraction directory created successfully");

    // Get info about the zip file
    unz_global_info globalInfo;
    if (unzGetGlobalInfo(zipFile, &globalInfo) != UNZ_OK) {
        brls::Logger::error("Failed to get global info from zip file");
        unzClose(zipFile);
        return false;
    }
    brls::Logger::debug("Zip file contains {} entries", globalInfo.number_entry);

    // Buffer for reading from zip file
    const int BUFFER_SIZE = 8192;
    char buffer[BUFFER_SIZE];

    // Initialize progress
    if (progressCallback) {
        progressCallback(0.0f);
    }

    // Extract each file
    for (uLong i = 0; i < globalInfo.number_entry; i++) {
        // Update progress
        if (progressCallback) {
            float progress = static_cast<float>(i) / static_cast<float>(globalInfo.number_entry);
            progressCallback(progress);
        }

        brls::Logger::debug("Processing entry {} of {}", i+1, globalInfo.number_entry);

        // Get info about current file
        unz_file_info fileInfo;
        char fileName[256];
        if (unzGetCurrentFileInfo(zipFile, &fileInfo, fileName, sizeof(fileName), NULL, 0, NULL, 0) != UNZ_OK) {
            brls::Logger::error("Failed to get file info for entry {}", i+1);
            unzClose(zipFile);
            return false;
        }

        brls::Logger::debug("Found file in zip: {}, size: {} bytes", fileName, fileInfo.uncompressed_size);

        // Clean up the filename (remove any leading slashes)
        std::string cleanFileName = fileName;
        while (!cleanFileName.empty() && cleanFileName[0] == '/') {
            cleanFileName = cleanFileName.substr(1);
        }

        // Construct the full path for extraction
        std::string fullPath;

        // Skip if the filename is empty
        if (cleanFileName.empty()) {
            brls::Logger::debug("Skipping empty filename");
            goto next_file;
        }

        // Set the full path now that we know the filename is not empty
        fullPath = extractPath + "/" + cleanFileName;
        brls::Logger::debug("Will extract to: {}", fullPath);

        // Check if this is a directory
        if (cleanFileName[cleanFileName.length() - 1] == '/') {
            brls::Logger::debug("Entry is a directory, creating: {}", fullPath);
            // Create directory
            if (!createDirRecursively(fullPath)) {
                brls::Logger::error("Failed to create directory: {}", fullPath);
                unzClose(zipFile);
                return false;
            }
            brls::Logger::debug("Directory created successfully: {}", fullPath);
        } else {
            // Create parent directories
            size_t pos = fullPath.find_last_of('/');
            if (pos != std::string::npos) {
                std::string dirPath = fullPath.substr(0, pos);
                brls::Logger::debug("Creating parent directory for file: {}", dirPath);
                if (!createDirRecursively(dirPath)) {
                    brls::Logger::error("Failed to create parent directory: {}", dirPath);
                    unzClose(zipFile);
                    return false;
                }
                brls::Logger::debug("Parent directory created successfully: {}", dirPath);
            }

            // Open the file in the zip
            brls::Logger::debug("Opening file in zip: {}", fileName);
            if (unzOpenCurrentFile(zipFile) != UNZ_OK) {
                brls::Logger::error("Failed to open file in zip: {}", fileName);
                unzClose(zipFile);
                return false;
            }
            brls::Logger::debug("Successfully opened file in zip: {}", fileName);

            // Open the output file
            brls::Logger::debug("Creating output file: {}", fullPath);
            FILE* outFile = fopen(fullPath.c_str(), "wb");
            if (!outFile) {
                brls::Logger::error("Failed to create output file: {}, error: {}", fullPath, strerror(errno));
                unzCloseCurrentFile(zipFile);
                unzClose(zipFile);
                return false;
            }
            brls::Logger::debug("Successfully created output file: {}", fullPath);

            // Read and write data
            brls::Logger::debug("Reading and writing file data for: {}", fileName);
            int bytesRead = 0;
            size_t totalBytesWritten = 0;

            do {
                bytesRead = unzReadCurrentFile(zipFile, buffer, BUFFER_SIZE);
                if (bytesRead < 0) {
                    brls::Logger::error("Error reading from zip file: {}, error code: {}", fileName, bytesRead);
                    fclose(outFile);
                    unzCloseCurrentFile(zipFile);
                    unzClose(zipFile);
                    return false;
                }

                if (bytesRead > 0) {
                    size_t result = fwrite(buffer, 1, bytesRead, outFile);
                    if (result != bytesRead) {
                        brls::Logger::error("Error writing to output file: {}, wrote {} of {} bytes, error: {}", 
                                           fullPath, result, bytesRead, strerror(errno));
                        fclose(outFile);
                        unzCloseCurrentFile(zipFile);
                        unzClose(zipFile);
                        return false;
                    }
                    totalBytesWritten += result;
                }
            } while (bytesRead > 0);

            brls::Logger::debug("Successfully wrote {} bytes to file: {}", totalBytesWritten, fullPath);

            // Close the output file
            brls::Logger::debug("Closing output file: {}", fullPath);
            fclose(outFile);

            // Close the current file in the zip
            brls::Logger::debug("Closing current file in zip: {}", fileName);
            int closeResult = unzCloseCurrentFile(zipFile);
            if (closeResult != UNZ_OK) {
                brls::Logger::error("Error closing file in zip: {}, error code: {}", fileName, closeResult);
                unzClose(zipFile);
                return false;
            }
        }

next_file:
        // Go to the next file if not at the end
        if ((i + 1) < globalInfo.number_entry) {
            brls::Logger::debug("Moving to next file in zip ({} of {})", i+2, globalInfo.number_entry);
            if (unzGoToNextFile(zipFile) != UNZ_OK) {
                brls::Logger::error("Failed to go to next file in zip");
                unzClose(zipFile);
                return false;
            }
        }
    }

    // Final progress update
    if (progressCallback) {
        progressCallback(1.0f);
    }

    // Close the zip file
    brls::Logger::debug("Closing zip file: {}", zipFilePath);
    unzClose(zipFile);
    brls::Logger::debug("Extraction completed successfully");
    return true;
}

// Helper function to check if there's an actual internet connection
bool hasInternetConnection() {
#ifdef __SWITCH__
    NifmInternetConnectionType type;
    u32 wifiSignal;
    NifmInternetConnectionStatus status;
    Result ret = nifmGetInternetConnectionStatus(&type, &wifiSignal, &status);

    // Check if we have a valid connection status and the connection is active
    return R_SUCCEEDED(ret) && 
           (type == NifmInternetConnectionType_WiFi || type == NifmInternetConnectionType_Ethernet) &&
           status == NifmInternetConnectionStatus_Connected;
#else
    // For other platforms, fall back to the platform's connectivity checks
    return brls::Application::getPlatform()->hasWirelessConnection() || 
           brls::Application::getPlatform()->hasEthernetConnection();
#endif
}

// Callback function to write received data to a string
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    std::string *response = (std::string *)userp;
    response->append((char *)contents, realsize);
    return realsize;
}

int checkForNewVersion(std::string& newVersion) {

    // Initialize socket system (should already be initialized by the app)
    bool needToInitSocket = false;

    // Try to initialize socket if needed
    Result rc = socketInitializeDefault();
    if (R_SUCCEEDED(rc)) {
        needToInitSocket = true;
    }

    CURL *curl;
    CURLcode res;
    std::string response;
    bool success = false;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (curl) {
        // Set the URL for the GitHub API
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/repos/insektaure/pkDex/releases/latest");

        // Set the User-Agent header
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "pkDex-Switch");

        // Set Accept header for GitHub API
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/vnd.github.v3+json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set the callback function to handle the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Follow redirects
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        res = curl_easy_perform(curl);

        // Clean up headers
        curl_slist_free_all(headers);

        if (res != CURLE_OK) {
            // do nothing
        } else {
            // Parse the response to extract the tag_name
            if (!response.empty()) {
                // Find the tag_name field in the JSON response
                size_t tag_pos = response.find("\"tag_name\":");
                if (tag_pos != std::string::npos) {
                    // Extract the version string
                    size_t start = response.find("\"", tag_pos + 11) + 1;
                    size_t end = response.find("\"", start);
                    if (start != std::string::npos && end != std::string::npos) {
                        newVersion = response.substr(start, end - start);
                        success = true;
                    }
                }
            }
        }

        // Clean up curl
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    // Clean up socket if we initialized it
    if (needToInitSocket) {
        socketExit();
    }

    if (!success) {
        return -1; // Failed to get or parse the response
    }

    // Compare with current version using the centralized version definition
    if (pkdex::isNewerVersion(pkdex::CURRENT_VERSION, newVersion)) {
        return 1; // New version available
    }

    return 0; // Current version is the latest
}

void checkForUpdatesAndNotify() {
    // Check if there's an actual internet connection
    if (hasInternetConnection()) {
        // Check for new version
        std::string newVersion;
        int result = checkForNewVersion(newVersion);
        if (result == 1) {
            brls::Application::notify("New version available: " + newVersion + " (Current: " + pkdex::CURRENT_VERSION + ")");
        }
    } else {
        // Notify the user when there's no network available
        brls::Application::notify("No network connection available. Unable to check for updates. #1");
    }
}

// Callback function for CURL to report download progress
static int progressCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    // Only update progress every 10% to avoid flooding the UI with notifications
    static int lastPercent = 0;

    // Reset lastPercent if download is just starting
    if (dlnow == 0) {
        lastPercent = 0;
    }

    int percent = (dltotal > 0) ? static_cast<int>((dlnow * 100) / dltotal) : 0;

    // Update progress every 10%, but skip the 100% notification to avoid multiple triggers
    if ((percent >= lastPercent + 10 && percent < 100) || (percent == 100 && lastPercent < 100)) {
        lastPercent = percent;

        // Check if clientp is NULL (should never happen with our fixes, but just to be safe)
        if (clientp == NULL) {
            return 0;
        }

        std::string* labelPtr = static_cast<std::string*>(clientp);
        std::string label = *labelPtr;

        brls::sync([percent, label]() {
            // Check if this is a version download or the high-res image pack
            if (label == "High-Res Pack") {
                brls::Application::notify("Downloading high-res image pack... " + std::to_string(percent) + "%");
            } else {
                brls::Application::notify("Downloading version " + label + "... " + std::to_string(percent) + "%");
            }
        });
    }

    return 0; // Return 0 to continue the download
}

bool downloadLatestVersion(const std::string& version) {
    // Construct the download URL
    std::string downloadUrl = "https://github.com/insektaure/pkDex/releases/download/" + version + "/pkDex.nro";

    // Define the filename where the update will be saved
    std::string filename = "/switch/pkDex.nro.new";

    // Check if the update file already exists
    struct stat buffer;
    bool fileExists = (stat(filename.c_str(), &buffer) == 0);

    if (fileExists) {
        // File exists, ask user if they want to redownload
        auto dialog = new brls::Dialog("An update file already exists. Do you want to redownload it ?");

        // Add redownload button
        dialog->addButton("Redownload", [version, downloadUrl, filename]() {
            // Delete the existing file
            if (remove(filename.c_str()) != 0) {
                brls::Application::notify("Failed to delete existing update file. Please delete it manually.");
                return;
            }

            // Notify user that download is starting
            brls::Application::notify("Downloading version " + version + "... (You can continue using the app)");

            // Use the generic download function
            downloadFile(
                downloadUrl,
                filename,
                version,
                []() {
                    brls::Application::notify("Download complete! Please run the pkDexUpdater to apply the update.");
                }
            );
        });

        // Add cancel button
        dialog->addButton("Cancel", []() {
            // Do nothing, dialog will close automatically
            brls::Application::notify("Download canceled. Existing update file kept.");
        });

        // Show the dialog
        dialog->open();
        return true;
    }

    // Notify user that download is starting
    brls::Application::notify("Downloading version " + version + "... (You can continue using the app)");

    // Use the generic download function
    return downloadFile(
        downloadUrl,
        filename,
        version,
        []() {
            brls::Application::notify("Download complete! Please run the pkDexUpdater to apply the update.");
        }
    );
}

// Generic download function that can be used by all download functions
bool downloadFile(const std::string& downloadUrl, const std::string& outputPath, 
                 const std::string& progressLabel, std::function<void()> onComplete) {
    // Check if there's an actual internet connection
    if (!hasInternetConnection()) {
        brls::Application::notify("No network connection available. Unable to download.");
        return false;
    }

    // Create a copy of the progress label for the progress callback
    std::string* labelCopy = new std::string(progressLabel);

    // Start the download in a background thread
    brls::async([downloadUrl, outputPath, labelCopy, onComplete]() {
        // Initialize variables
        CURL *curl = NULL;
        CURLcode res;
        FILE *fp = NULL;
        bool success = false;
        bool needToInitSocket = false;

        // Try to initialize socket if needed
        Result rc = socketInitializeDefault();
        if (R_SUCCEEDED(rc)) {
            needToInitSocket = true;
        }

        // Open file for writing
        fp = fopen(outputPath.c_str(), "wb");
        if (!fp) {
            brls::sync([outputPath]() {
                brls::Application::notify("Failed to create download file: " + outputPath);
            });
            if (needToInitSocket) {
                socketExit();
            }
            delete labelCopy; // Clean up
            return;
        }

        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();

        if (curl) {
            // Set the URL
            curl_easy_setopt(curl, CURLOPT_URL, downloadUrl.c_str());

            // Set the User-Agent header
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "pkDex-Switch");

            // Follow redirects
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

            // Write data to file
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

            // Set up progress callback
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, labelCopy);

            // Set a timeout to prevent hanging on slow connections
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L); // No timeout for the transfer itself

            // Set buffer size to prevent memory issues
            curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L); // 100KB buffer

            // Perform the request
            res = curl_easy_perform(curl);

            // Check for errors
            if (res != CURLE_OK) {
                std::string error = curl_easy_strerror(res);
                brls::sync([error]() {
                    brls::Application::notify("Download failed: " + error);
                });
            } else {
                success = true;

                // Call the completion callback if provided
                if (onComplete) {
                    brls::sync(onComplete);
                }
            }

            // Clean up curl
            curl_easy_cleanup(curl);
            curl = NULL;
        }

        curl_global_cleanup();

        // Close file
        if (fp) {
            fclose(fp);
            fp = NULL;
        }

        // Clean up socket if we initialized it
        if (needToInitSocket) {
            socketExit();
        }

        // Clean up the label copy
        delete labelCopy;
    });

    // Return true immediately since the download is happening in the background
    return true;
}

// Helper function to start the download in a background thread (kept for backward compatibility)
void startDownload(const std::string& version, const std::string& downloadUrl, std::string* versionCopy) {
    // Use the new generic download function
    downloadFile(
        downloadUrl, 
        "/switch/pkDex.nro.new", 
        *versionCopy,
        []() {
            brls::Application::notify("Download complete! Please run the pkDexUpdater to apply the update.");
        }
    );

    // Clean up the version copy since downloadFile creates its own copy
    delete versionCopy;
}

bool downloadUpdater(const std::string& version) {
    // Check for the latest version available
    std::string latestVersion;
    int result = checkForNewVersion(latestVersion);

    // If failed to get latest version, use the provided version
    if (result == -1) {
        brls::Application::notify("Failed to check for latest version. Using current version instead.");
        latestVersion = version;
    } else if (result == 0) {
        // Current version is the latest
        latestVersion = version;
    }
    // else result == 1, latestVersion contains the new version

    // Construct the download URL using the latest version
    std::string downloadUrl = "https://github.com/insektaure/pkDex/releases/download/" + latestVersion + "/pkDexUpdater.nro";

    // Notify user that download is starting
    brls::Application::notify("Downloading updater from version " + latestVersion + "... (You can continue using the app)");

    // Use the generic download function
    return downloadFile(
        downloadUrl,
        "/switch/pkDexUpdater.nro",
        latestVersion,
        []() {
            brls::Application::notify("Updater download complete! You can now launch the updater.");
        }
    );
}

bool downloadHighResImagePack() {
    // Define the filename where the high-res image pack will be saved
    std::string filename = "/pkDex_High_Res_imgs.zip";

    // URL for the high-res image pack
    std::string downloadUrl = "https://github.com/insektaure/pkDex/releases/latest/download/pkDex_High_Res_imgs.zip";

    // Check if the file already exists
    struct stat buffer;
    bool fileExists = (stat(filename.c_str(), &buffer) == 0);

    if (fileExists) {
        // File exists, ask user if they want to redownload
        auto dialog = new brls::Dialog("High-resolution image pack already exists.\n\nDo you want to redownload it ?");

        // Add redownload button
        dialog->addButton("Redownload", [filename, downloadUrl]() {
            // Delete the existing file
            if (remove(filename.c_str()) != 0) {
                brls::Application::notify("Failed to delete existing file. Please delete it manually.");
                return;
            }

            // Notify user that download is starting
            brls::Application::notify("Downloading high-res image pack... (You can continue using the app)");

            // Use the generic download function
            downloadFile(
                downloadUrl,
                filename,
                "High-Res Pack",
                []() {
                    // Create a dialog to inform the user that the download is complete
                    auto dialog = new brls::Dialog("Download complete !\n\nHigh-resolution images are now available for extraction.\n\nUse the 'Extract High-Res Image Pack' button in Settings.");

                    // Add OK button
                    dialog->addButton("OK", []() {
                        // Dialog will close automatically
                    });

                    // Show the dialog
                    dialog->open();
                }
            );
        });

        // Add cancel button
        dialog->addButton("Cancel", []() {
            // Do nothing, dialog will close automatically
            brls::Application::notify("Download canceled. Existing file kept.");
        });

        // Show the dialog
        dialog->open();
        return true;
    }

    // Notify user that download is starting
    brls::Application::notify("Downloading high-res image pack... (You can continue using the app)");

    // Use the generic download function
    return downloadFile(
        downloadUrl,
        filename,
        "High-Res Pack",
        []() {
            // Create a dialog to inform the user that the download is complete
            auto dialog = new brls::Dialog("Download complete !\n\nHigh-resolution images are now available for extraction.\n\nUse the 'Extract High-Res Image Pack' button in Settings.");

            // Add OK button
            dialog->addButton("OK", []() {
                // Dialog will close automatically
            });

            // Show the dialog
            dialog->open();
        }
    );
}

bool extractHighResImagePack() {
    // Define the filename where the high-res image pack is saved
    std::string zipFilePath = "/pkDex_High_Res_imgs.zip";
    std::string extractPath = "/";

    // Check if the zip file exists
    struct stat buffer;
    if (stat(zipFilePath.c_str(), &buffer) != 0) {
        brls::Application::notify("High-resolution image pack not found. Please download it first.");
        return false;
    }

    // Show a dialog to confirm extraction
    auto dialog = new brls::Dialog("Extract high-resolution image pack ?\n\nThis may take a while.");

    // Add extract button
    dialog->addButton("Extract", [zipFilePath, extractPath]() {
        // Create a box to hold the progress components
        brls::Box* progressBox = new brls::Box();
        progressBox->setAxis(brls::Axis::COLUMN);
        progressBox->setHeight(200);
        progressBox->setWidth(600);
        progressBox->setMargins(0, 0, 0, 55);  // Remove margins to allow proper centering
        // Set padding between elements
        progressBox->setPadding(24);  // Use padding instead of spacing
        // Center content vertically and stretch horizontally
        progressBox->setJustifyContent(brls::JustifyContent::CENTER);
        progressBox->setAlignItems(brls::AlignItems::STRETCH);

        // Add a title label
        brls::Label* titleLabel = new brls::Label();
        titleLabel->setText("Extracting...");
        titleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        titleLabel->setFontSize(24);

        // Create a label to show progress percentage
        brls::Label* progressLabel = new brls::Label();
        progressLabel->setText("0%");
        progressLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        progressLabel->setFontSize(24);

        // Create a slider to show progress
        brls::Slider* progressBar = new brls::Slider();
        progressBar->setWidthPercentage(100.0f);  // Make the slider take up the full width
        progressBar->setProgress(0.0f);
        // Set the slider to be non-interactive
        progressBar->setFocusable(false);
        // Set the slider pointer size to 0 to hide it
        progressBar->setPointerSize(0);

        // Add components to the box
        progressBox->addView(titleLabel);
        progressBox->addView(progressBar);
        progressBox->addView(progressLabel);

        // Create a progress dialog with the box as content
        auto progressDialog = new brls::Dialog(progressBox);

        // Make the dialog non-cancelable so it can't be closed before extraction is complete
        progressDialog->setCancelable(false);

        // Open the progress dialog
        progressDialog->open();

        // Start extraction in a background thread
        brls::async([zipFilePath, extractPath, progressBar, progressLabel, progressDialog]() {
            // Ensure the base directory exists
            brls::Logger::debug("Ensuring base directory exists: /switch/pkDex");
            if (!createDirRecursively("/switch/pkDex")) {
                brls::Logger::error("Failed to create base directory: /switch/pkDex");
                brls::sync([progressDialog]() {
                    progressDialog->close();
                    brls::Application::notify("Extraction failed : Could not create base directory.");
                });
                return;
            }

            // Extract the zip file with progress updates
            bool extractSuccess = extractZipFile(zipFilePath, extractPath, [progressBar, progressLabel](float progress) {
                // Update the progress bar and label on the main thread
                brls::sync([progressBar, progressLabel, progress]() {
                    progressBar->setProgress(progress);
                    int percentage = static_cast<int>(progress * 100);
                    progressLabel->setText(std::to_string(percentage) + "%");
                });
            });

            // Close the progress dialog and show the result
            brls::sync([progressDialog, extractSuccess, zipFilePath]() {
                progressDialog->close();

                if (!extractSuccess) {
                    auto resultDialog = new brls::Dialog("Extraction failed. Please try again or extract the zip file manually.");
                    resultDialog->addButton("OK", []() {
                        // Dialog will close automatically
                    });
                    resultDialog->open();
                } else {
                    // Ask user if they want to keep the downloaded zip file
                    auto resultDialog = new brls::Dialog("Extraction complete !\nHigh-resolution images are now available.\n\nDo you want to keep the downloaded zip file ?");

                    // Add "Keep" button
                    resultDialog->addButton("Keep", []() {
                        // Do nothing, just keep the file
                        brls::Application::notify("Zip file has been kept.");
                    });

                    // Add "Delete" button
                    resultDialog->addButton("Delete", [zipFilePath]() {
                        // Delete the zip file
                        if (std::remove(zipFilePath.c_str()) == 0) {
                            brls::Application::notify("Zip file has been deleted.");
                        } else {
                            brls::Application::notify("Failed to delete the zip file.");
                        }
                    });

                    resultDialog->open();
                }
            });
        });
    });

    // Add cancel button
    dialog->addButton("Cancel", []() {
        // Do nothing, dialog will close automatically
    });

    // Show the dialog
    dialog->open();
    return true;
}

bool manualCheckForUpdates() {
    // Check if there's an actual internet connection
    if (!hasInternetConnection()) {
        brls::Application::notify("No network connection available. Unable to check for updates. #2");
        return false;
    }

    // Check for new version
    std::string newVersion;
    int result = checkForNewVersion(newVersion);

    if (result == 1) {
        // New version available - show confirmation dialog
        auto dialog = new brls::Dialog("New version available: " + newVersion + " (Current: " + pkdex::CURRENT_VERSION + ")");

        // Add download button
        dialog->addButton("Download", [newVersion]() {
            downloadLatestVersion(newVersion);
        });

        // Add cancel button
        dialog->addButton("Cancel", []() {
            // Do nothing, dialog will close automatically
        });

        // Show the dialog
        dialog->open();
    } else if (result == 0) {
        // Current version is the latest
        brls::Application::notify("You are using the latest version: " + pkdex::CURRENT_VERSION);
    } else {
        // Network error or other failure
        brls::Application::notify("Failed to check for updates. Please check your network connection and try again.");
        return false;
    }

    return true;
}
