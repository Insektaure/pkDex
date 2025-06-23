#pragma once

#include <borealis.hpp>

class SettingsTab : public brls::Box
{
  public:
    SettingsTab();

    BRLS_BIND(brls::DetailCell, checkUpdates, "checkUpdates");

    static brls::View* create();
};
