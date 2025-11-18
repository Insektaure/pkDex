#pragma once

#include <borealis.hpp>

class SettingsTab : public brls::Box
{
  public:
    SettingsTab();

    BRLS_BIND(brls::DetailCell, checkUpdates, "checkUpdates");
    BRLS_BIND(brls::DetailCell, launchUpdater, "launchUpdater");
    BRLS_BIND(brls::SelectorCell, regionSelector, "regionSelector");
    BRLS_BIND(brls::SelectorCell, localeSelector, "localeSelector");
    BRLS_BIND(brls::DetailCell, resetCaptureStatus, "resetCaptureStatus");
    BRLS_BIND(brls::BooleanCell, toggleCheckVersionOnLaunch, "toggleCheckVersionOnLaunch");
    BRLS_BIND(brls::BooleanCell, toggleHideBottomBar, "toggleHideBottomBar");
    BRLS_BIND(brls::DetailCell, downloadHighResImages, "downloadHighResImages");
    BRLS_BIND(brls::DetailCell, extractHighResImages, "extractHighResImages");

    static brls::View* create();

  private:
    // When true, we will show the restart dialog after the locale dropdown closes
    bool pendingLocaleRestartPrompt = false;
};
