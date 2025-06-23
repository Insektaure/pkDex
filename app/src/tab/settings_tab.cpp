#include "tab/settings_tab.hpp"
#include "update_checker.hpp"
#include <string>
#include <switch.h>

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
}

brls::View* SettingsTab::create()
{
    return new SettingsTab();
}
