#pragma once

#include <borealis.hpp>

class ComponentsTab : public brls::Box
{
  public:
    ComponentsTab();

    static brls::View* create();

  private:
    BRLS_BIND(brls::Label, progress, "progress");
    BRLS_BIND(brls::Slider, slider, "slider");
    bool onPrimaryButtonClicked(brls::View* view);
};
