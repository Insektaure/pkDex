#include "tab/changelog_tab.hpp"

#include <borealis/core/i18n.hpp>
#include <borealis/core/logger.hpp>
#include <unistd.h>

using namespace brls::literals;

ChangelogTab::ChangelogTab()
{
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/tabs/changelog.xml");

    // Load the changelog content
    loadChangelogContent();
}

void ChangelogTab::loadChangelogContent()
{
    // Try to load the changelog from the SD card first
    std::string sdPath = "/switch/pkDex/changelog.txt";
    std::string embeddedPath = "romfs:/changelog.txt";
    std::string content;
    bool fileLoaded = false;

    // Check if file exists on SD card
    if (access(sdPath.c_str(), F_OK) != -1)
    {
        // File exists on SD card, try to read it
        std::ifstream file(sdPath);
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            content = buffer.str();
            fileLoaded = true;
            file.close();
        }
    }

    // If not loaded from SD card, try embedded resource
    if (!fileLoaded)
    {
        std::ifstream file(embeddedPath);
        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            content = buffer.str();
            fileLoaded = true;
            file.close();
        }
    }

    // Set the content to the label
    if (fileLoaded)
    {
        changelog_content->setText(content);
    }
    else
    {
        changelog_content->setText("Changelog file not found.");
    }
}

brls::View* ChangelogTab::create()
{
    // Called by the XML engine to create a new ChangelogTab
    return new ChangelogTab();
}