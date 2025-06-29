#pragma once

#include <borealis.hpp>
#include <fstream>
#include <sstream>

class ChangelogTab : public brls::Box
{
  public:
    ChangelogTab();

    static brls::View* create();

  private:
    void loadChangelogContent();
    
    BRLS_BIND(brls::Label, changelog_content, "changelog_content");
};