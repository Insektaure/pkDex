#pragma once

#include <borealis.hpp>
#include "tab/settings_tab.hpp"

class MainActivity : public brls::Activity
{
  public:
    // Declare that the content of this activity is the given XML file
    CONTENT_FROM_XML_RES("activity/main.xml");

    void onContentAvailable() override;
};
