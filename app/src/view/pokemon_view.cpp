/*
    Copyright 2021 XITRIX

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "view/pokemon_view.hpp"

#include <borealis/core/i18n.hpp>
#include <borealis/core/logger.hpp>
#include <unistd.h>

using namespace brls::literals;

bool dismissView(brls::View* view, PokemonView* pock)
{
    return true;
}

PokemonView::PokemonView(Pokemon pokemon)
    : pokemon(pokemon)
{
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/views/pokemon.xml");

    auto dismissAction = [this](View* view) {
        this->dismiss();
        return true;
    };

    brls::Label* label = new brls::Label();
    label->setText(brls::Hint::getKeyIcon(brls::ControllerButton::BUTTON_RB) + " Закрыть");
    label->setFontSize(24);
    label->setMargins(0, 12, 0, 12);

    brls::Box* holder = new brls::Box();
    holder->addView(label);
    holder->setFocusable(true);
    holder->addGestureRecognizer(new brls::TapGestureRecognizer(holder));

    holder->registerClickAction(dismissAction);
    holder->registerAction("Close", brls::ControllerButton::BUTTON_RB, dismissAction, true);
    registerAction("Close", brls::ControllerButton::BUTTON_RB, dismissAction, true);

    getAppletFrameItem()->title = pokemon.name;
    getAppletFrameItem()->setIconFromRes("img/pokemon/icons/" + pokemon.id + ".png");
//    getAppletFrameItem()->hintView = holder;

    // Set images
    loadHighResImage(standard_image, "img/pokemon/full", pokemon.id);
    loadHighResImage(shiny_image, "img/pokemon/shiny", pokemon.id);

    // Set label texts
    standard_label->setText("");
    shiny_label->setText("");

    // Set text fields
    national_dex->setText(pokemon.id);
    regional_dex->setText(pokemon.regionalDexNumber);
    type->setText(pokemon.type);
    evolution->setText(pokemon.evolution);
    exclusive_version->setText(pokemon.exclusiveVersion);

    // Process locations text to handle <br> tags
    std::string locationsText = pokemon.locations;
    size_t pos = 0;
    while ((pos = locationsText.find("<br>", pos)) != std::string::npos) {
        locationsText.replace(pos, 4, "\n");
        pos += 1; // Move past the replacement
    }
    locations->setText(locationsText);

    // Set up close button action
    close_button->registerClickAction([this](brls::View* view) {
        this->dismiss();
        return true;
    });
}

void PokemonView::loadHighResImage(brls::Image* image, const std::string& path, const std::string& id)
{
    std::string sdPath = "/switch/pkDex/resources/" + path + "/" + id + ".png";
    if (access(sdPath.c_str(), F_OK) != -1) {
        // File exists on SD card
        //brls::Logger::info("Loading high-res image from SD card: {}", sdPath);
        image->setImageFromFile(sdPath);
    } else {
        // Fall back to embedded resource
        //brls::Logger::info("High-res image not found on SD card, using embedded resource: {}", path + "/" + id + ".png");
        image->setImageFromRes(path + "/" + id + ".png");
    }
}

brls::View* PokemonView::create()
{
    // Called by the XML engine to create a new ComponentsTab
    return new PokemonView();
}
