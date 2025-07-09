#include "tab/settings_tab.hpp"
#include "update_checker.hpp"
#include "config.hpp"
#include "version.hpp"
#include "data/pokemon_tracker.hpp"
#include <string>
#include <switch.h>
#include <sys/stat.h>
#include <borealis/core/i18n.hpp>

// Function to check if a file exists
bool fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}


// Function to launch the updater application
void launchUpdaterApp() {
    // Path to the updater application
    const char* updaterPath = "/switch/pkDexUpdater.nro";

    // Check if the updater file exists
    if (!fileExists(updaterPath)) {
        // Show a dialog with option to download the updater
        auto errorDialog = new brls::Dialog("The updater application is missing. Would you like to download it now?");

        // Add Download button
        errorDialog->addButton("Download", []() {
            // Get the current version to use for downloading
            std::string version = pkdex::CURRENT_VERSION;
            downloadUpdater(version);
        });

        // Add Cancel button
        errorDialog->addButton("Cancel", []() {
            // Do nothing, dialog will close automatically
        });

        // Show the dialog
        errorDialog->open();
        return;
    }

    // Show a confirmation dialog
    auto dialog = new brls::Dialog("This will close pkDex and launch the updater. Make sure you have downloaded an update first.");

    // Add confirm button
    dialog->addButton("Launch", [updaterPath]() {
        // Set the next application to load when this one exits
        envSetNextLoad(updaterPath, "");

        // Exit the application
        brls::Application::quit();
    });

    // Add cancel button
    dialog->addButton("Cancel", []() {
        // Do nothing, dialog will close automatically
    });

    // Show the dialog
    dialog->open();
}

using namespace brls::literals;  // for _i18n

SettingsTab::SettingsTab()
{
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/tabs/settings.xml");

    // Register click action for the "Check for updates" button
    checkUpdates->registerClickAction([](...){
        manualCheckForUpdates();
        return true;
    });

    // Register click action for the "Launch updater" button
    launchUpdater->registerClickAction([](...){
        launchUpdaterApp();
        return true;
    });

    // Register click action for the "Reset Pokémon Capture Status" button
    resetCaptureStatus->registerClickAction([this](...){
        // Get the selected region index
        int selectedRegionIndex = regionSelector->getSelection();

        // Initialize the region selector (same as in constructor)
        std::vector<std::string> regionIds = pkdex::PokemonTracker::getAllRegions();
        regionIds.insert(regionIds.begin(), "all_regions");

        // Get display names for regions
        std::vector<std::string> regionDisplayNames;
        regionDisplayNames.push_back("pkdex/settings/regions/all_regions"_i18n);
        for (size_t i = 1; i < regionIds.size(); i++) {
            std::string regionId = regionIds[i];
            std::string i18nKey = "pkdex/settings/regions/" + regionId;
            regionDisplayNames.push_back(brls::getStr(i18nKey));
        }

        // Determine if we're resetting all regions or a specific one
        bool resetAll = (selectedRegionIndex == 0); // "All Regions" is at index 0

        // Create the confirmation message
        std::string message;
        if (resetAll) {
            message = "pkdex/settings/reset_confirm_all"_i18n;
        } else {
            // Get the selected region display name
            std::string regionDisplayName = regionDisplayNames[selectedRegionIndex];
            std::string messageTemplate = "pkdex/settings/reset_confirm_region"_i18n;
            // Replace {region} placeholder with the actual region name
            size_t pos = messageTemplate.find("{region}");
            if (pos != std::string::npos) {
                messageTemplate.replace(pos, 8, regionDisplayName);
            }
            message = messageTemplate;
        }

        // Show a confirmation dialog
        auto dialog = new brls::Dialog(message);

        // Add confirm button
        dialog->addButton("pkdex/settings/reset_button"_i18n, [resetAll, selectedRegionIndex, regionIds, regionDisplayNames]() {
            bool success;

            if (resetAll) {
                // Reset all capture statuses for all regions
                success = pkdex::PokemonTracker::resetAllCaptureStatus();
            } else {
                // Reset capture statuses for the selected region
                std::string regionId = regionIds[selectedRegionIndex];
                success = pkdex::PokemonTracker::resetRegionCaptureStatus(regionId);
            }

            // Show a success or error message
            if (success) {
                if (resetAll) {
                    brls::Application::notify("pkdex/settings/reset_success_all"_i18n);
                } else {
                    std::string regionDisplayName = regionDisplayNames[selectedRegionIndex];
                    std::string messageTemplate = "pkdex/settings/reset_success_region"_i18n;
                    // Replace {region} placeholder with the actual region name
                    size_t pos = messageTemplate.find("{region}");
                    if (pos != std::string::npos) {
                        messageTemplate.replace(pos, 8, regionDisplayName);
                    }
                    brls::Application::notify(messageTemplate);
                }
            } else {
                brls::Application::notify("pkdex/settings/reset_failure"_i18n);
            }
        });

        // Add cancel button
        dialog->addButton("pkdex/settings/cancel_button"_i18n, []() {
            // Do nothing, dialog will close automatically
        });

        // Show the dialog
        dialog->open();
        return true;
    });

    // Initialize the toggle with the current setting from the config file
    bool checkVersionOnLaunch = pkdex::Config::getBool("toggle_check_version_on_launch", true);
    toggleCheckVersionOnLaunch->init("Check for updates on launch", checkVersionOnLaunch, [](bool value) {
        // Save the setting to the config file
        pkdex::Config::setBool("toggle_check_version_on_launch", value);
        return value;
    });

    // Initialize the region selector
    std::vector<std::string> regionIds = pkdex::PokemonTracker::getAllRegions();
    std::vector<std::string> regionNames;

    // Add "All Regions" as the first option with internationalized name
    regionIds.insert(regionIds.begin(), "all_regions");
    regionNames.push_back("pkdex/settings/regions/all_regions"_i18n);

    // Add internationalized names for each region
    for (size_t i = 1; i < regionIds.size(); i++) {
        std::string regionId = regionIds[i];
        std::string i18nKey = "pkdex/settings/regions/" + regionId;
        regionNames.push_back(brls::getStr(i18nKey));
    }

    // Get the currently selected region from config, default to "All Regions" (index 0)
    int selectedRegionIndex = pkdex::Config::getInt("selected_region_index", 0);
    if (selectedRegionIndex >= regionNames.size()) {
        selectedRegionIndex = 0; // Reset to "All Regions" if the saved index is invalid
    }

    regionSelector->init("pkdex/settings/select_region_to_reset"_i18n, regionNames, selectedRegionIndex, [](int selected) {
        // Save the selected region index to config
        pkdex::Config::setInt("selected_region_index", selected);
    });

    // Initialize the toggle for hiding the bottom bar
    bool hideBottomBar = pkdex::Config::getBool("toggle_hide_bottom_bar", false);
    toggleHideBottomBar->init("Hide Bottom Bar", hideBottomBar, [](bool value) {
        // Save the setting to the config file
        pkdex::Config::setBool("toggle_hide_bottom_bar", value);

        // Update the bottom bar visibility
        brls::AppletFrame::HIDE_BOTTOM_BAR = value;

        // Apply the change to all active frames
        auto stack = brls::Application::getActivitiesStack();
        for (auto& activity : stack) {
            auto* frame = dynamic_cast<brls::AppletFrame*>(
                activity->getContentView());
            if (!frame) continue;
            frame->setFooterVisibility(value ? brls::Visibility::GONE
                                             : brls::Visibility::VISIBLE);
        }

        return value;
    });

    // Register click action for the "Download High-Res Image Pack" button
    downloadHighResImages->registerClickAction([](...){
        downloadHighResImagePack();
        return true;
    });

    // Register click action for the "Extract High-Res Image Pack" button
    extractHighResImages->registerClickAction([](...){
        extractHighResImagePack();
        return true;
    });
}

brls::View* SettingsTab::create()
{
    return new SettingsTab();
}
