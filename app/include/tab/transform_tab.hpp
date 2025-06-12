#pragma once

#include <borealis.hpp>

class TransformBox: public brls::Image {
  public:
    void draw(NVGcontext* vg, float x, float y, float width, float height, brls::Style style, brls::FrameContext* ctx) override;

    static brls::View* create();

    void setRotate(float deg);

    void setSkewX(float deg);

    void setSkewY(float deg);

    void setScaleX(float size);

    void setScaleY(float size);

    void setFontScaleX(float size);

    void setFontScaleY(float size);

  private:
    float skew_x{}, skew_y{}, rotate{}, scale_x{1}, scale_y{1};
    float font_scale_x{1}, font_scale_y{1};
};

class TransformTab : public brls::Box
{
  public:
    TransformTab();

    static brls::View* create();

  private:
    BRLS_BIND(TransformBox, box, "box");
    BRLS_BIND(brls::SliderCell, transX, "transX");
    BRLS_BIND(brls::SliderCell, transY, "transY");
    BRLS_BIND(brls::SliderCell, scaleX, "scaleX");
    BRLS_BIND(brls::SliderCell, scaleY, "scaleY");
    BRLS_BIND(brls::SliderCell, skewX, "skewX");
    BRLS_BIND(brls::SliderCell, skewY, "skewY");
    BRLS_BIND(brls::SliderCell, rotate, "rotate");
    BRLS_BIND(brls::SliderCell, boxWidth, "width");
    BRLS_BIND(brls::SliderCell, boxHeight, "height");
    BRLS_BIND(brls::SliderCell, fontScaleX, "fontScaleX");
    BRLS_BIND(brls::SliderCell, fontScaleY, "fontScaleY");
    BRLS_BIND(brls::Button, reset, "reset");
    BRLS_BIND(brls::Button, play, "play");

    static void registerCell(brls::SliderCell* cell, float init, const std::string& title, const std::function<float(float)>& cb);

    brls::Animatable aniX, aniY, skew, skew2;
};
