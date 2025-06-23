#include "activity/main_activity.hpp"
#include "tab/settings_tab.hpp"

void MainActivity::onContentAvailable()
{
    // Call parent method to load the XML content
    brls::Activity::onContentAvailable();

    // Get the TabFrame from the content
    brls::TabFrame* tabFrame = (brls::TabFrame*)this->getContentView()->getView("tabFrame");
    if (!tabFrame)
    {
        brls::Logger::error("Could not find tabFrame in main.xml");
        return;
    }

    // Add a separator
    tabFrame->addSeparator();

    // Add the settings tab
    tabFrame->addTab("Settings", []() {
        return new SettingsTab();
    });
}
