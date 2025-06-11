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

    // Calculate the current absolute index
    int currentIndex = currentSelection.section * 30 + currentSelection.row;

    // Calculate the new absolute index (jump back by 15 rows)
    int newIndex = std::max(0, currentIndex - 15);

    // Calculate the new section and row
    int newSection = newIndex / 30;
    int newRow = newIndex % 30;

    // Create the new index path
    brls::IndexPath newIndexPath(newSection, newRow);

    // Update the data source's current selection
    this->dataSource->setCurrentSelection(newIndexPath);

    // Ensure the cell is fully visible
    ensureCellVisible(newIndexPath);

    return true;
}

bool RecyclingListTab::jumpToNextPage(brls::View* view)
{
    // Get the current selection
    brls::IndexPath currentSelection = this->dataSource->getCurrentSelection();

    // Calculate the total number of rows
    int totalRows = pokemons.size();

    // Calculate the current absolute index
    int currentIndex = currentSelection.section * 30 + currentSelection.row;

    // Calculate the new absolute index (jump forward by 15 rows)
    int newIndex = std::min(totalRows - 1, currentIndex + 15);

    // Calculate the new section and row
    int newSection = newIndex / 30;
    int newRow = newIndex % 30;

    // Create the new index path
    brls::IndexPath newIndexPath(newSection, newRow);

    // Update the data source's current selection
    this->dataSource->setCurrentSelection(newIndexPath);

    // Ensure the cell is fully visible
    ensureCellVisible(newIndexPath);

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

void RecyclingListTab::ensureCellVisible(const brls::IndexPath& indexPath)
{
    // First, use selectRowAt to scroll to the cell (which tries to center it in the view)
    recycler->selectRowAt(indexPath, true);

    // Get the estimated row height (this is a rough approximation)
    float estimatedRowHeight = recycler->estimatedRowHeight;

    // Get the current content offset
    float currentOffset = recycler->getContentOffsetY();

    // Get the height of the recycler frame
    float frameHeight = recycler->getHeight();

    // Calculate the estimated position of the cell
    float estimatedCellTop = currentOffset + (frameHeight / 2) - (estimatedRowHeight / 2);
    float estimatedCellBottom = estimatedCellTop + estimatedRowHeight;

    // Check if the estimated cell position is within the visible frame
    if (estimatedCellTop < currentOffset)
    {
        // Cell might be partially above the visible area
        // Add a larger offset to ensure it's fully visible
        recycler->setContentOffsetY(currentOffset - (estimatedRowHeight / 2), true);
    }
    else if (estimatedCellBottom > currentOffset + frameHeight)
    {
        // Cell might be partially below the visible area
        // Add a larger offset to ensure it's fully visible
        recycler->setContentOffsetY(currentOffset + (estimatedRowHeight / 2), true);
    }

    // Force a refresh of the recycler to ensure all cells are properly positioned
    recycler->invalidate();

    // Give focus to the recycler itself to ensure joystick scrolling works properly
    // This will trigger the recycler's focus handling mechanism
    brls::Application::giveFocus(recycler);
}
