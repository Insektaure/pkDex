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
DataSource::DataSource()
    : currentSelection(0, 0) // Initialize with first section, first row
{
}

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
    // Update the current selection
    currentSelection = indexPath;

    // Calculate the actual index based on section and row, same as in cellForRow
    int actualIndex = indexPath.section * 30 + indexPath.row;
    recycler->present(new PokemonView(pokemons[actualIndex]));
}

// RECYCLER VIEW

RecyclingListTab::RecyclingListTab()
    : RecyclingListTab("paldea") // Default to Paldea region
{
}

RecyclingListTab::RecyclingListTab(const std::string& region)
{
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/tabs/recycling_list.xml");

    // Load Pokemon data from the specified region
    loadPokemonData(region);

    recycler->estimatedRowHeight = 70;
    recycler->registerCell("Header", []() { return RecyclerHeader::create(); });
    recycler->registerCell("Cell", []() { return RecyclerCell::create(); });

    // Create and set the data source
    this->dataSource = new DataSource();
    recycler->setDataSource(this->dataSource);

    // Register L and R button actions for jumping between pages
    this->registerAction("Jump to Previous Page", brls::BUTTON_LB, 
        std::bind(&RecyclingListTab::jumpToPreviousPage, this, std::placeholders::_1), false, true);
    this->registerAction("Jump to Next Page", brls::BUTTON_RB, 
        std::bind(&RecyclingListTab::jumpToNextPage, this, std::placeholders::_1), false, true);
}

void RecyclingListTab::loadPokemonData(const std::string& region)
{
    pokemons.clear();
    // Load Pokemon data from the specified region
    pokemons = PokemonDataLoader::loadPokemonFromRegion(region);
}

bool RecyclingListTab::jumpToPreviousPage(brls::View* view)
{
    // Get the current selection
    brls::IndexPath currentSelection = this->dataSource->getCurrentSelection();

    // Calculate the total number of rows
    int totalRows = pokemons.size();

    // If we're already at the first section, stay there
    if (currentSelection.section == 0)
    {
        // If we're not at the first row, go to the first row of the current section
        if (currentSelection.row > 0)
        {
            brls::IndexPath newIndexPath(0, 0);
            // Smooth scroll to the first row
            recycler->selectRowAt(newIndexPath, true);
            // Update the current selection in the data source
            this->dataSource->setCurrentSelection(newIndexPath);
        }
        return true;
    }

    // Scroll to the first row of the previous section
    int newSection = currentSelection.section - 1;
    int newRow = 0;

    // Create the new index path
    brls::IndexPath newIndexPath(newSection, newRow);

    // First update the data source's current selection
    this->dataSource->setCurrentSelection(newIndexPath);

    // Then smooth scroll to the new row
    // This will scroll to the row without changing focus immediately
    recycler->selectRowAt(newIndexPath, true);

    return true;
}

bool RecyclingListTab::jumpToNextPage(brls::View* view)
{
    // Get the current selection
    brls::IndexPath currentSelection = this->dataSource->getCurrentSelection();

    // Calculate the total number of sections
    int totalSections = (pokemons.size() + 29) / 30; // Same calculation as in DataSource::numberOfSections

    // If we're already at the last section
    if (currentSelection.section == totalSections - 1)
    {
        // Calculate the number of rows in the last section
        int rowsInLastSection = pokemons.size() - (totalSections - 1) * 30;

        // If we're not at the last row, go to the last row of the current section
        if (currentSelection.row < rowsInLastSection - 1)
        {
            brls::IndexPath newIndexPath(currentSelection.section, rowsInLastSection - 1);
            // First update the data source's current selection
            this->dataSource->setCurrentSelection(newIndexPath);

            // Then smooth scroll to the last row
            // This will scroll to the row without changing focus immediately
            recycler->selectRowAt(newIndexPath, true);
        }
        return true;
    }

    // Scroll to the first row of the next section
    int newSection = currentSelection.section + 1;
    int newRow = 0;

    // Create the new index path
    brls::IndexPath newIndexPath(newSection, newRow);

    // First update the data source's current selection
    this->dataSource->setCurrentSelection(newIndexPath);

    // Then smooth scroll to the new row
    // This will scroll to the row without changing focus immediately
    recycler->selectRowAt(newIndexPath, true);

    return true;
}

brls::View* RecyclingListTab::create()
{
    // Called by the XML engine to create a new RecyclingListTab with default region
    return new RecyclingListTab("paldea");
}

brls::View* RecyclingListTab::create(const std::string& region)
{
    // Create a new RecyclingListTab with the specified region
    return new RecyclingListTab(region);
}

// Factory methods for specific regions
brls::View* RecyclingListTab::createKanto()
{
    return new RecyclingListTab("kanto");
}

brls::View* RecyclingListTab::createJohto()
{
    return new RecyclingListTab("johto");
}

brls::View* RecyclingListTab::createHoenn()
{
    return new RecyclingListTab("hoenn");
}

brls::View* RecyclingListTab::createSinnoh()
{
    return new RecyclingListTab("sinnoh");
}

brls::View* RecyclingListTab::createSinnohArceus()
{
    return new RecyclingListTab("sinnoh_arceus");
}

brls::View* RecyclingListTab::createGalar()
{
    return new RecyclingListTab("galar");
}

brls::View* RecyclingListTab::createPaldea()
{
    return new RecyclingListTab("paldea");
}
