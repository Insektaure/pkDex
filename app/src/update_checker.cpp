#include "update_checker.hpp"
#include "version.hpp"
#include <borealis.hpp>
#include <switch.h>
#include <string>
#include <cstring>
#include <curl/curl.h>
#include <unistd.h>

// Callback function to write received data to a string
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    std::string *response = (std::string *)userp;
    response->append((char *)contents, realsize);
    return realsize;
}

bool checkForNewVersion(std::string& newVersion) {
    // Check if network is available
    if (!brls::Application::getPlatform()->hasWirelessConnection() && 
        !brls::Application::getPlatform()->hasEthernetConnection()) {
        return false;
    }

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
        return false;
    }

    // Compare with current version using the centralized version definition
    if (pkdex::isNewerVersion(pkdex::CURRENT_VERSION, newVersion)) {
        return true; // New version available
    }

    return false; // No new version available
}

void checkForUpdatesAndNotify() {
    // Check if network is connected (either WiFi or Ethernet)
    if (brls::Application::getPlatform()->hasWirelessConnection() || 
        brls::Application::getPlatform()->hasEthernetConnection()) {
        // Check for new version
        std::string newVersion;
        if (checkForNewVersion(newVersion)) {
            brls::Application::notify("New version available: " + newVersion + " (Current: " + pkdex::CURRENT_VERSION + ")");
        }
    }
}

bool manualCheckForUpdates() {
    // Check if network is connected (either WiFi or Ethernet)
    if (!brls::Application::getPlatform()->hasWirelessConnection() && 
        !brls::Application::getPlatform()->hasEthernetConnection()) {
        brls::Application::notify("No network connection available. Please connect to WiFi or Ethernet to check for updates.");
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
