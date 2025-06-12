#include "tab/text_test_tab.hpp"

static void registerSliderCell(brls::SliderCell* cell, float init, const std::string& title, const std::function<int(float)>& cb)
{
    int res = cb(init);
    cell->setDetailText(res == 0 ? "auto" : fmt::format("{}", res));
    cell->slider->setPointerSize(20);
    cell->init(title, init, [cb, cell](float value)
        {
            int res = cb(value);
            cell->setDetailText(res == 0 ? "auto" : fmt::format("{}", res)); });
}

TextTestTab::TextTestTab()
{
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/tabs/text_test.xml");

    registerSliderCell(width, 1.0f, "width", [this](float value)
        {
            value *= 400;
            label->setWidth(value <= 0.0f ? brls::View::AUTO : value); return value; });

    registerSliderCell(height, 0.0f, "height", [this](float value)
        {
            value *= 400;
            singleLine->setVisibility(value <= 0.0f ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
            label->setHeight(value <= 0.0f ? brls::View::AUTO : value); return value; });

    vertical->init("verticalAlign", { "baseline", "top", "center", "bottom" }, 2, [this](int value)
        {
            label->setVerticalAlign((brls::VerticalAlign)value); return value; });

    horizontal->init("horizontalAlign", { "left", "center", "right" }, 0, [this](int value)
        {
            label->setHorizontalAlign((brls::HorizontalAlign)value); return value; });

    singleLine->init("singleLine", false, [this](bool value)
        {
            label->setSingleLine(value); return value; });
}

brls::View* TextTestTab::create()
{
    return new TextTestTab();
}