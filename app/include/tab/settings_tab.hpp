#pragma once

#include <borealis.hpp>

class SettingsTab : public brls::Box
{
  public:
    SettingsTab();

    BRLS_BIND(brls::DetailCell, checkUpdates, "checkUpdates");
    BRLS_BIND(brls::DetailCell, launchUpdater, "launchUpdater");
    BRLS_BIND(brls::SelectorCell, regionSelector, "regionSelector");
    BRLS_BIND(brls::DetailCell, resetCaptureStatus, "resetCaptureStatus");
    BRLS_BIND(brls::BooleanCell, toggleCheckVersionOnLaunch, "toggleCheckVersionOnLaunch");
    BRLS_BIND(brls::BooleanCell, toggleHideBottomBar, "toggleHideBottomBar");
    BRLS_BIND(brls::DetailCell, downloadHighResImages, "downloadHighResImages");
    BRLS_BIND(brls::DetailCell, extractHighResImages, "extractHighResImages");

    // Helper function to create a confirmation dialog with confirm/cancel buttons
    static brls::Dialog* createConfirmDialog(
        const std::string& message,
        const std::string& confirmText,
        const std::string& cancelText,
        std::function<void()> confirmAction,
        std::function<void()> cancelAction = nullptr);

    // Helper function to replace placeholders in a string
    static std::string replacePlaceholder(
        const std::string& text,
        const std::string& placeholder,
        const std::string& replacement);

    static brls::View* create();
};
