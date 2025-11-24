#include "tab/recycling_list_tab.hpp"
#include "view/pokemon_view.hpp"
#include "data/pokemon_data_loader.hpp"
#include "data/pokemon_tracker.hpp"
#include "borealis/extern/nanovg/nanovg.h"

using namespace brls::literals; // for _i18n

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

    // Get the current region from the parent tab
    std::string region = "";
    RecyclingListTab* parentTab = dynamic_cast<RecyclingListTab*>(recycler->getParent());
    if (parentTab) {
        region = parentTab->getCurrentRegion();
    } else {
        // Default to paldea if we can't get the region
        region = "paldea";
    }



    // Get the capture states
    pkdex::CaptureStates captureStates = pkdex::PokemonTracker::getCaptureStates(region, pokemons[actualIndex].regionalDexNumber);

    RecyclerCell* item = (RecyclerCell*)recycler->dequeueReusableCell("Cell");

    // Set the cell text without prefix
    item->label->setText(pokemons[actualIndex].regionalDexNumber + " - " + pokemons[actualIndex].name);
    item->image->setImageFromRes("img/pokemon/icons/" + pokemons[actualIndex].id + ".png");

    // Show all icons for tracked states (horizontal layout)
    std::string icons = "";
    if (captureStates.normal)
        icons += "img/pokeball.png;";
    if (captureStates.shiny)
        icons += "img/pokeball_shiny.png;";
    if (captureStates.alpha)
        icons += "img/pokeball_alpha.png;";
    if (captureStates.shinyAlpha)
        icons += "img/pokeball_shiny_alpha.png;";

    // Remove trailing semicolon
    if (!icons.empty() && icons.back() == ';')
        icons.pop_back();

    // If no icons, hide the rightIcon
    if (icons.empty()) {
        item->rightIcon->setImageFromRes("");
        item->rightIcon->setVisibility(brls::Visibility::GONE);
    } else {
        // If only one icon, show it
        size_t sep = icons.find(';');
        if (sep == std::string::npos) {
            item->rightIcon->setImageFromRes(icons);
            item->rightIcon->setVisibility(brls::Visibility::VISIBLE);
        } else {
            // If multiple icons, show the first and add a badge or indicator (for now, just show the first)
            item->rightIcon->setImageFromRes(icons.substr(0, sep));
            item->rightIcon->setVisibility(brls::Visibility::VISIBLE);
            // TODO: For full support, extend RecyclerCell to show multiple icons horizontally
        }
    }

    return item;
}

void DataSource::didSelectRowAt(brls::RecyclerFrame* recycler, brls::IndexPath indexPath)
{
    // Update the current selection
    currentSelection = indexPath;

    // Get the current region from the parent tab
    std::string region = "paldea"; // Default to paldea if we can't get the region
    RecyclingListTab* parentTab = dynamic_cast<RecyclingListTab*>(recycler->getParent());
    if (parentTab) {
        region = parentTab->getCurrentRegion();    }

    // Calculate the actual index based on section and row, same as in cellForRow
    int actualIndex = indexPath.section * 30 + indexPath.row;
    recycler->present(new PokemonView(pokemons[actualIndex], actualIndex, region));
}

// RECYCLER VIEW

RecyclingListTab::RecyclingListTab()
    : RecyclingListTab("paldea") // Default to Paldea region
{
}

RecyclingListTab::RecyclingListTab(const std::string& region)
    : currentRegion(region)
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
    this->registerAction("pkdex/listing/previous_page"_i18n, brls::BUTTON_LB,
        std::bind(&RecyclingListTab::jumpToPreviousPage, this, std::placeholders::_1), false, true);
    this->registerAction("pkdex/listing/next_page"_i18n, brls::BUTTON_RB,
        std::bind(&RecyclingListTab::jumpToNextPage, this, std::placeholders::_1), false, true);

    // Register Y button action for toggling capture status
    this->registerAction("pkdex/listing/toggle_capture_status"_i18n, brls::BUTTON_Y,
        std::bind(&RecyclingListTab::toggleCaptureStatus, this, std::placeholders::_1), false, true);
}

void RecyclingListTab::loadPokemonData(const std::string& region)
{
    // Update the current region
    this->currentRegion = region;

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

    // Calculate the new absolute index (jump back by 30 rows)
    int newIndex = std::max(0, currentIndex - 30);

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

    // Calculate the new absolute index (jump forward by 30 rows)
    int newIndex = std::min(totalRows - 1, currentIndex + 30);

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

brls::View* RecyclingListTab::createIsleArmor()
{
    return new RecyclingListTab("isle_armor");
}

brls::View* RecyclingListTab::createCrownTundra()
{
    return new RecyclingListTab("crown_tundra");
}

brls::View* RecyclingListTab::createPaldea()
{
    return new RecyclingListTab("paldea");
}

brls::View* RecyclingListTab::createKitakami()
{
    return new RecyclingListTab("kitakami");
}

brls::View* RecyclingListTab::createBlueberry()
{
    return new RecyclingListTab("blueberry_academy");
}

brls::View* RecyclingListTab::createKalosLza()
{
    return new RecyclingListTab("kalos_lza");
}

bool RecyclingListTab::toggleCaptureStatus(brls::View* view)
{
    // Get the currently focused view
    brls::View* focusedView = brls::Application::getCurrentFocus();

    // Try to cast it to a RecyclerCell
    brls::RecyclerCell* focusedCell = dynamic_cast<brls::RecyclerCell*>(focusedView);

    // If the focused view is a RecyclerCell, use its index path
    brls::IndexPath currentSelection;
    if (focusedCell)
    {
        currentSelection = focusedCell->getIndexPath();
    }
    else
    {
        // Fallback to the data source's current selection
        currentSelection = this->dataSource->getCurrentSelection();
    }

    // Calculate the actual index based on section and row
    int actualIndex = currentSelection.section * 30 + currentSelection.row;

    // Get the Pokemon at the current index
    Pokemon& pokemon = pokemons[actualIndex];

    // Get current capture states
    pkdex::CaptureStates states = pkdex::PokemonTracker::getCaptureStates(currentRegion, pokemon.regionalDexNumber);

    // Create dialog text with current state
    std::string dialogText = "Toggle capture states for " + pokemon.name + ":\n";
    dialogText += (states.normal ? "[X] " : "[ ] ") + std::string("Normal\n");
    dialogText += (states.shiny ? "[X] " : "[ ] ") + std::string("Shiny\n");
    dialogText += (states.alpha ? "[X] " : "[ ] ") + std::string("Alpha\n");
    dialogText += (states.shinyAlpha ? "[X] " : "[ ] ") + std::string("Shiny Alpha");

    brls::Dialog* dialog = new brls::Dialog(dialogText);
    dialog->addButton(states.normal ? "Unset Normal" : "Set Normal", [=]() {
        float contentOffset = recycler->getContentOffsetY();
        pkdex::PokemonTracker::toggleCaptureState(currentRegion, pokemon.regionalDexNumber, 0);
        brls::Application::notify(pokemon.name + (states.normal ? " normal unset" : " normal set"));
        recycler->reloadData();
        recycler->setDefaultCellFocus(currentSelection);
        this->dataSource->setCurrentSelection(currentSelection);
        recycler->setContentOffsetY(contentOffset, false);
        recycler->selectRowAt(currentSelection, false);
        recycler->invalidate();
        brls::Application::giveFocus(recycler);
    });
    dialog->addButton(states.shiny ? "Unset Shiny" : "Set Shiny", [=]() {
        float contentOffset = recycler->getContentOffsetY();
        pkdex::PokemonTracker::toggleCaptureState(currentRegion, pokemon.regionalDexNumber, 1);
        brls::Application::notify(pokemon.name + (states.shiny ? " shiny unset" : " shiny set"));
        recycler->reloadData();
        recycler->setDefaultCellFocus(currentSelection);
        this->dataSource->setCurrentSelection(currentSelection);
        recycler->setContentOffsetY(contentOffset, false);
        recycler->selectRowAt(currentSelection, false);
        recycler->invalidate();
        brls::Application::giveFocus(recycler);
    });
    dialog->addButton(states.alpha ? "Unset Alpha" : "Set Alpha", [=]() {
        float contentOffset = recycler->getContentOffsetY();
        pkdex::PokemonTracker::toggleCaptureState(currentRegion, pokemon.regionalDexNumber, 2);
        brls::Application::notify(pokemon.name + (states.alpha ? " alpha unset" : " alpha set"));
        recycler->reloadData();
        recycler->setDefaultCellFocus(currentSelection);
        this->dataSource->setCurrentSelection(currentSelection);
        recycler->setContentOffsetY(contentOffset, false);
        recycler->selectRowAt(currentSelection, false);
        recycler->invalidate();
        brls::Application::giveFocus(recycler);
    });
    // If you want a 4th button, you may need to use a custom dialog or menu, but for now, use a notification for shiny alpha
    dialog->setCancelable(true);
    dialog->open();

    return true;
}

void RecyclingListTab::ensureCellVisible(const brls::IndexPath& indexPath)
{
    // Use selectRowAt to select the cell but don't center it in the view
    recycler->selectRowAt(indexPath, false);

    // Get the estimated row height (this is a rough approximation)
    float estimatedRowHeight = recycler->estimatedRowHeight;

    // Get the current content offset
    float currentOffset = recycler->getContentOffsetY();

    // Get the height of the recycler frame
    float frameHeight = recycler->getHeight();

    // Calculate the estimated position of the cell
    float estimatedCellPosition = indexPath.section * 30 * estimatedRowHeight + indexPath.row * estimatedRowHeight;

    // Check if the cell is outside the visible area
    if (estimatedCellPosition < currentOffset)
    {
        // Cell is above the visible area, scroll to make it visible at the top
        recycler->setContentOffsetY(estimatedCellPosition, true);
    }
    else if (estimatedCellPosition + estimatedRowHeight > currentOffset + frameHeight)
    {
        // Cell is below the visible area, scroll to make it visible at the bottom
        recycler->setContentOffsetY(estimatedCellPosition + estimatedRowHeight - frameHeight, true);
    }

    // Force a refresh of the recycler to ensure all cells are properly positioned
    recycler->invalidate();

    // Give focus to the recycler itself to ensure joystick scrolling works properly
    // This will trigger the recycler's focus handling mechanism
    brls::Application::giveFocus(recycler);
}
