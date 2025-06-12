#pragma once

#include <borealis.hpp>

class TextTestTab : public brls::Box
{
  public:
    TextTestTab();

    static brls::View* create();

  private:
    BRLS_BIND(brls::SliderCell, width, "width");
    BRLS_BIND(brls::SliderCell, height, "height");
    BRLS_BIND(brls::BooleanCell, singleLine, "singleLine");
    BRLS_BIND(brls::SelectorCell, horizontal, "horizontal");
    BRLS_BIND(brls::SelectorCell, vertical, "vertical");
    BRLS_BIND(brls::Label, label, "label");
};
