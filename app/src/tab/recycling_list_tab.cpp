/*
    Copyright 2020-2021 natinusala

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

#include "tab/recycling_list_tab.hpp"
#include "view/pokemon_view.hpp"
#include "data/pokemon_data_loader.hpp"

std::vector<Pokemon> pokemons;

RecyclerCell::RecyclerCell()
{
    this->inflateFromXMLRes("xml/cells/cell.xml");
}

RecyclerCell* RecyclerCell::create()
{
    return new RecyclerCell();
}

// DATA SOURCE

int DataSource::numberOfSections(brls::RecyclerFrame* recycler)
{
    return (pokemons.size() + 29) / 30; // Round up to account for remaining rows
}

int DataSource::numberOfRows(brls::RecyclerFrame* recycler, int section)
{
    int startIndex = section * 30;
    int remainingRows = pokemons.size() - startIndex;
    return std::min(remainingRows, 30); // Return up to 30 rows per section
}

std::string DataSource::titleForHeader(brls::RecyclerFrame* recycler, int section) 
{
    int startIndex = section * 30 + 1; // Start index for the section (1-based)
    int endIndex = std::min((section + 1) * 30, (int)pokemons.size()); // End index for the section
    return std::to_string(startIndex) + " - " + std::to_string(endIndex);
}

brls::RecyclerCell* DataSource::cellForRow(brls::RecyclerFrame* recycler, brls::IndexPath indexPath)
{
    // Calculate the actual index based on section and row
    int actualIndex = indexPath.section * 30 + indexPath.row;

    RecyclerCell* item = (RecyclerCell*)recycler->dequeueReusableCell("Cell");
    item->label->setText(pokemons[actualIndex].regionalDexNumber + " - " + pokemons[actualIndex].name);
    item->image->setImageFromRes("img/pokemon/icons/" + pokemons[actualIndex].id + ".png");
    return item;
}

void DataSource::didSelectRowAt(brls::RecyclerFrame* recycler, brls::IndexPath indexPath)
{
    // Calculate the actual index based on section and row, same as in cellForRow
    int actualIndex = indexPath.section * 30 + indexPath.row;
    recycler->present(new PokemonView(pokemons[actualIndex]));
}

// RECYCLER VIEW

RecyclingListTab::RecyclingListTab()
{
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/tabs/recycling_list.xml");

    pokemons.clear();
    // Load Pokemon data from the Kanto region
    pokemons = PokemonDataLoader::loadPokemonFromRegion("paldea");

    recycler->estimatedRowHeight = 70;
    recycler->registerCell("Header", []() { return RecyclerHeader::create(); });
    recycler->registerCell("Cell", []() { return RecyclerCell::create(); });
    recycler->setDataSource(new DataSource());
}

brls::View* RecyclingListTab::create()
{
    // Called by the XML engine to create a new RecyclingListTab
    return new RecyclingListTab();
}
