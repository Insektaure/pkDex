#include "tab/settings_tab.hpp"
#include "update_checker.hpp"
#include "config.hpp"
#include "version.hpp"
#include "data/pokemon_tracker.hpp"
#include <string>
#include <switch.h>
#include <sys/stat.h>

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
    resetCaptureStatus->registerClickAction([](...){
        // Show a confirmation dialog
        auto dialog = new brls::Dialog("Are you sure you want to reset all Pokemon capture statuses? This action cannot be undone.");

        // Add confirm button
        dialog->addButton("Reset", []() {
            // Reset all capture statuses
            bool success = pkdex::PokemonTracker::resetAllCaptureStatus();

            // Show a success or error message
            if (success) {
                brls::Application::notify("All Pokemon capture statuses have been reset.");
            } else {
                brls::Application::notify("Failed to reset Pokemon capture statuses.");
            }
        });

        // Add cancel button
        dialog->addButton("Cancel", []() {
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
}

brls::View* SettingsTab::create()
{
    return new SettingsTab();
}
