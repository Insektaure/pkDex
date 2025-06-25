#pragma once

#include <borealis.hpp>

class SettingsTab : public brls::Box
{
  public:
    SettingsTab();

    BRLS_BIND(brls::DetailCell, checkUpdates, "checkUpdates");
    BRLS_BIND(brls::DetailCell, launchUpdater, "launchUpdater");
    BRLS_BIND(brls::DetailCell, resetCaptureStatus, "resetCaptureStatus");
    BRLS_BIND(brls::BooleanCell, toggleCheckVersionOnLaunch, "toggleCheckVersionOnLaunch");

    static brls::View* create();
};
