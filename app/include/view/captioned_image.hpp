#pragma once

#include <borealis.hpp>

class CaptionedImage : public brls::Box
{
  public:
    CaptionedImage();

    void onChildFocusGained(brls::View* directChild, brls::View* focusedView) override;
    void onChildFocusLost(brls::View* directChild, brls::View* focusedView) override;

    static brls::View* create();

  private:
    BRLS_BIND(brls::Image, image, "image");
    BRLS_BIND(brls::Label, label, "label");
};
