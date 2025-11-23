#include "tab/settings_tab.hpp"
#include "update_checker.hpp"
#include "config.hpp"
#include "version.hpp"
#include "data/pokemon_tracker.hpp"
#include <string>
#include <switch.h>
#include <sys/stat.h>
#include <borealis/core/i18n.hpp>

using namespace brls::literals; // for _i18n

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
        auto errorDialog = new brls::Dialog("pkdex/settings/updater_app_missing"_i18n);

        // Add Download button
        errorDialog->addButton("pkdex/common/download"_i18n, []() {
            // Get the current version to use for downloading
            std::string version = pkdex::CURRENT_VERSION;
            downloadUpdater(version);
        });

        // Add Cancel button
        errorDialog->addButton("pkdex/common/cancel"_i18n, []() {
            // Do nothing, dialog will close automatically
        });

        // Show the dialog
        errorDialog->open();
        return;
    }

    // Show a confirmation dialog
    auto dialog = new brls::Dialog("pkdex/settings/updater_app_launch"_i18n);

    // Add confirm button
    dialog->addButton("pkdex/common/launch"_i18n, [updaterPath]() {
        // Set the next application to load when this one exits
        envSetNextLoad(updaterPath, "");

        // Exit the application
        brls::Application::quit();
    });

    // Add cancel button
    dialog->addButton("pkdex/common/cancel"_i18n, []() {
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
    toggleCheckVersionOnLaunch->init("pkdex/settings/check_updates_on_launch"_i18n, checkVersionOnLaunch, [](bool value) {
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

    // Initialize the locale selector
    // Supported locales list (value) and their display names (label)
    std::vector<std::string> localeValues = {"en-US", "fr-FR", "de-DE", "es-ES", "it-IT", "ja-JP"};
    std::vector<std::string> localeLabels = {
        "pkdex/settings/locales/en-US"_i18n,
        "pkdex/settings/locales/fr-FR"_i18n,
        "pkdex/settings/locales/de-DE"_i18n,
        "pkdex/settings/locales/es-ES"_i18n,
        "pkdex/settings/locales/it-IT"_i18n,
        "pkdex/settings/locales/ja-JP"_i18n
    };

    // Load current preference (default: en-US)
    std::string savedLocale = pkdex::Config::getString("i18n_locale", "en-US");
    int savedIndex = 0;
    for (size_t i = 0; i < localeValues.size(); i++)
    {
        if (localeValues[i] == savedLocale)
        {
            savedIndex = (int)i;
            break;
        }
    }

    localeSelector->init(
        "pkdex/settings/select_locale"_i18n,
        localeLabels,
        savedIndex,
        // on selection
        [this, localeValues](int selected) {
            // Persist selected locale code
            if (selected >= 0 && selected < (int)localeValues.size())
                pkdex::Config::setString("i18n_locale", localeValues[selected]);

            // Mark to show restart prompt after the dropdown dismisses
            this->pendingLocaleRestartPrompt = true;
        },
        // on dismiss (called after the dropdown is closed)
        [this](int /*selected*/) {
            if (!this->pendingLocaleRestartPrompt)
                return;
            this->pendingLocaleRestartPrompt = false;

            auto dialog = new brls::Dialog("pkdex/settings/locale_restart_message"_i18n);
            dialog->addButton("pkdex/settings/restart_now"_i18n, []() {
                brls::Application::quit();
            });
            dialog->addButton("pkdex/settings/restart_later"_i18n, []() {});
            dialog->open();
        }
    );

    // Initialize the toggle for hiding the bottom bar
    bool hideBottomBar = pkdex::Config::getBool("toggle_hide_bottom_bar", false);
    toggleHideBottomBar->init("pkdex/settings/hide_bottom_bar"_i18n, hideBottomBar, [](bool value) {
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
