#pragma once

#include <borealis.hpp>

class SettingsTab : public brls::Box
{
  public:
    SettingsTab();

    BRLS_BIND(brls::RadioCell, radio, "radio");
    BRLS_BIND(brls::BooleanCell, boolean, "boolean");
    BRLS_BIND(brls::SelectorCell, selector, "selector");
    BRLS_BIND(brls::InputCell, input, "input");
    BRLS_BIND(brls::InputNumericCell, inputNumeric, "inputNumeric");
    BRLS_BIND(brls::DetailCell, ipAddress, "ipAddress");
    BRLS_BIND(brls::DetailCell, dnsServer, "dnsServer");
    BRLS_BIND(brls::BooleanCell, debug, "debug");
    BRLS_BIND(brls::BooleanCell, bottomBar, "bottomBar");
    BRLS_BIND(brls::BooleanCell, alwaysOnTop, "alwaysOnTop");
    BRLS_BIND(brls::BooleanCell, fps, "fps");
    BRLS_BIND(brls::SelectorCell, swapInterval, "swapInterval");
    BRLS_BIND(brls::SliderCell, slider, "slider");
    BRLS_BIND(brls::DetailCell, notify, "notify");

    static brls::View* create();
};
