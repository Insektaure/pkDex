#include "tab/settings_tab.hpp"
#include "update_checker.hpp"
#include "config.hpp"
#include <string>
#include <switch.h>

// Function to launch the updater application
void launchUpdaterApp() {
    // Path to the updater application
    const char* updaterPath = "/switch/pkDexUpdater.nro";

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
