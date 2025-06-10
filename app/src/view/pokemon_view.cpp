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
    standard_image->setImageFromRes("img/pokemon/full/" + pokemon.id + ".png");
    shiny_image->setImageFromRes("img/pokemon/shiny/" + pokemon.id + ".png");

    // Set text fields
    national_dex->setText(pokemon.id);
    regional_dex->setText(pokemon.regionalDexNumber);
    type->setText(pokemon.type);
    evolution->setText(pokemon.evolution);
    exclusive_version->setText(pokemon.exclusiveVersion);
    locations->setText(pokemon.locations);

}

brls::View* PokemonView::create()
{
    // Called by the XML engine to create a new ComponentsTab
    return new PokemonView();
}
