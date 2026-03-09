#include "tab/recycling_list_tab.hpp"
#include "view/pokemon_view.hpp"
#include "data/pokemon_data_loader.hpp"
#include "data/pokemon_tracker.hpp"
#include "utils/region_registry.hpp"
#include "utils/string_utils.hpp"
#include "borealis/extern/nanovg/nanovg.h"

using namespace brls::literals; // for _i18n

std::vector<Pokemon> RecyclingListTab::s_pokemons;

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
    return (RecyclingListTab::getPokemons().size() + 29) / 30; // Round up to account for remaining rows
}

int DataSource::numberOfRows(brls::RecyclerFrame* recycler, int section)
{
    int startIndex = section * 30;
    int remainingRows = RecyclingListTab::getPokemons().size() - startIndex;
    return std::min(remainingRows, 30); // Return up to 30 rows per section
}

std::string DataSource::titleForHeader(brls::RecyclerFrame* recycler, int section)
{
    int startIndex = section * 30 + 1; // Start index for the section (1-based)
    int endIndex = std::min((section + 1) * 30, (int)RecyclingListTab::getPokemons().size()); // End index for the section
    return std::to_string(startIndex) + " - " + std::to_string(endIndex);
}

brls::RecyclerCell* DataSource::cellForRow(brls::RecyclerFrame* recycler, brls::IndexPath indexPath)
{
    // Calculate the actual index based on section and row
    int actualIndex = indexPath.section * 30 + indexPath.row;

    auto& pokemons = RecyclingListTab::getPokemons();

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

    // Show/hide multi-select indicator
    if (parentTab && parentTab->isMultiSelectMode() && parentTab->isIndexSelected(actualIndex)) {
        item->selectIndicator->setColor(nvgRGB(0, 255, 200));
        item->selectIndicator->setVisibility(brls::Visibility::VISIBLE);
    } else {
        item->selectIndicator->setVisibility(brls::Visibility::GONE);
    }

    // Set up to four icons for tracked states
    std::vector<std::string> iconPaths;
    if (captureStates.normal)
        iconPaths.push_back("img/pokeball.png");
    if (captureStates.shiny)
        iconPaths.push_back("img/shiny.png");
    if (captureStates.alpha)
        iconPaths.push_back("img/alpha.png");
    if (captureStates.shinyAlpha)
        iconPaths.push_back("img/shiny_alpha.png");

    // Set all icons invisible by default
    item->rightIcon->setVisibility(brls::Visibility::GONE);
    item->rightIcon2->setVisibility(brls::Visibility::GONE);
    item->rightIcon3->setVisibility(brls::Visibility::GONE);
    item->rightIcon4->setVisibility(brls::Visibility::GONE);

    // Show icons in order
    if (iconPaths.size() > 0) {
        item->rightIcon->setImageFromRes(iconPaths[0]);
        item->rightIcon->setVisibility(brls::Visibility::VISIBLE);
    }
    if (iconPaths.size() > 1) {
        item->rightIcon2->setImageFromRes(iconPaths[1]);
        item->rightIcon2->setVisibility(brls::Visibility::VISIBLE);
    }
    if (iconPaths.size() > 2) {
        item->rightIcon3->setImageFromRes(iconPaths[2]);
        item->rightIcon3->setVisibility(brls::Visibility::VISIBLE);
    }
    if (iconPaths.size() > 3) {
        item->rightIcon4->setImageFromRes(iconPaths[3]);
        item->rightIcon4->setVisibility(brls::Visibility::VISIBLE);
    }

    return item;
}

void DataSource::didSelectRowAt(brls::RecyclerFrame* recycler, brls::IndexPath indexPath)
{
    // Update the current selection
    currentSelection = indexPath;

    auto& pokemons = RecyclingListTab::getPokemons();

    // Get the parent tab
    RecyclingListTab* parentTab = dynamic_cast<RecyclingListTab*>(recycler->getParent());

    // Calculate the actual index based on section and row
    int actualIndex = indexPath.section * 30 + indexPath.row;

    // In multi-select mode, A button toggles selection instead of opening detail
    if (parentTab && parentTab->isMultiSelectMode()) {
        parentTab->toggleIndexSelection(actualIndex);
        // Update just this cell's indicator without full reload
        brls::View* focusedView = brls::Application::getCurrentFocus();
        RecyclerCell* cell = dynamic_cast<RecyclerCell*>(focusedView);
        if (cell) {
            if (parentTab->isIndexSelected(actualIndex)) {
                cell->selectIndicator->setColor(nvgRGB(0, 255, 200));
                cell->selectIndicator->setVisibility(brls::Visibility::VISIBLE);
            } else {
                cell->selectIndicator->setVisibility(brls::Visibility::GONE);
            }
        }
        return;
    }

    // Normal mode: open detail view
    std::string region = "paldea";
    if (parentTab) {
        region = parentTab->getCurrentRegion();
    }
    recycler->present(new PokemonView(pokemons[actualIndex], actualIndex, region));
}

// DIALOG HELPERS

std::pair<brls::Box*, brls::Dialog*> RecyclingListTab::createMenuDialog(const std::string& title)
{
    auto* menuBox = new brls::Box();
    menuBox->setAxis(brls::Axis::COLUMN);
    menuBox->setPadding(24);

    auto* titleLabel = new brls::Label();
    titleLabel->setText(title);
    titleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    titleLabel->setFontSize(28);
    titleLabel->setMaxHeight(48);
    menuBox->addView(titleLabel);

    auto* spacer = new brls::Box();
    spacer->setAxis(brls::Axis::COLUMN);
    spacer->setMinHeight(24);
    spacer->setMaxHeight(24);
    menuBox->addView(spacer);

    auto* menuDialog = new brls::Dialog(menuBox);
    menuDialog->setCancelable(true);

    return {menuBox, menuDialog};
}

void RecyclingListTab::addMenuSeparator(brls::Box* menuBox)
{
    auto* separator = new brls::Box();
    separator->setAxis(brls::Axis::COLUMN);
    separator->setMinHeight(16);
    separator->setMaxHeight(16);
    menuBox->addView(separator);
}

void RecyclingListTab::registerDialogClose(brls::Dialog* dialog)
{
    dialog->registerAction("close"_i18n, brls::BUTTON_B, [dialog](brls::View*) {
        dialog->close([]{});
        return true;
    }, true);
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

    // Style the multi-select label
    multiSelectLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    multiSelectLabel->setFontSize(18);
    multiSelectLabel->setMargins(8, 16, 8, 16);

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

    // Register X button action for bulk actions
    this->registerAction("pkdex/listing/bulk_actions"_i18n, brls::BUTTON_X,
        std::bind(&RecyclingListTab::openBulkActionsDialog, this, std::placeholders::_1), false, true);

    // Register ZL button action for toggling multi-select mode
    this->registerAction("pkdex/listing/multi_select"_i18n, brls::BUTTON_LT,
        std::bind(&RecyclingListTab::toggleMultiSelectMode, this, std::placeholders::_1), false, true);
}

void RecyclingListTab::loadPokemonData(const std::string& region)
{
    // Update the current region
    this->currentRegion = region;

    s_pokemons.clear();
    // Load Pokemon data from the specified region
    s_pokemons = PokemonDataLoader::loadPokemonFromRegion(region);
}

bool RecyclingListTab::jumpToPreviousPage(brls::View* view)
{
    // Get the current selection
    brls::IndexPath currentSelection = this->dataSource->getCurrentSelection();

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
    int totalRows = s_pokemons.size();

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

brls::IndexPath RecyclingListTab::getFocusedIndexPath()
{
    brls::View* focusedView = brls::Application::getCurrentFocus();
    brls::RecyclerCell* focusedCell = dynamic_cast<brls::RecyclerCell*>(focusedView);
    if (focusedCell)
        return focusedCell->getIndexPath();
    return this->dataSource->getCurrentSelection();
}

void RecyclingListTab::refreshRecycler(const brls::IndexPath& sel)
{
    float contentOffset = recycler->getContentOffsetY();
    recycler->reloadData();
    recycler->invalidate();
    recycler->setDefaultCellFocus(sel);
    this->dataSource->setCurrentSelection(sel);
    recycler->setContentOffsetY(contentOffset, false);
    recycler->selectRowAt(sel, false);
    brls::Application::giveFocus(recycler);
}

void RecyclingListTab::updateMultiSelectLabel()
{
    if (multiSelectMode) {
        std::string text = "pkdex/listing/multi_select"_i18n;
        text += " — " + std::to_string(selectedIndices.size()) + " / " + std::to_string(s_pokemons.size());
        multiSelectLabel->setText(text);
        multiSelectLabel->setVisibility(brls::Visibility::VISIBLE);
    } else {
        multiSelectLabel->setVisibility(brls::Visibility::GONE);
    }
}

bool RecyclingListTab::toggleMultiSelectMode(brls::View* view)
{
    multiSelectMode = !multiSelectMode;

    if (!multiSelectMode) {
        exitMultiSelectMode();
    } else {
        updateMultiSelectLabel();
    }
    return true;
}

void RecyclingListTab::toggleIndexSelection(int index)
{
    if (selectedIndices.count(index) > 0)
        selectedIndices.erase(index);
    else
        selectedIndices.insert(index);
    updateMultiSelectLabel();
}

void RecyclingListTab::exitMultiSelectMode()
{
    brls::IndexPath sel = getFocusedIndexPath();
    multiSelectMode = false;
    selectedIndices.clear();
    updateMultiSelectLabel();
    refreshRecycler(sel);
}

bool RecyclingListTab::toggleCaptureStatus(brls::View* view)
{
    brls::IndexPath currentSelection = getFocusedIndexPath();
    bool hasAlpha = pkdex::regionHasAlpha(currentRegion);

    // Multi-select mode: apply to all selected Pokemon
    if (multiSelectMode && !selectedIndices.empty()) {
        std::vector<std::string> selectedDexNumbers;
        for (int idx : selectedIndices) {
            selectedDexNumbers.push_back(s_pokemons[idx].regionalDexNumber);
        }

        int selectedCount = selectedDexNumbers.size();
        std::string region = currentRegion;

        std::string title = "pkdex/capture_toggle/title"_i18n;
        title += " (" + std::to_string(selectedCount) + ")";
        auto [menuBox, menuDialog] = createMenuDialog(title);

        auto addMultiToggle = [=](const std::string& label, int stateIndex, bool value) {
            auto* cell = new brls::DetailCell();
            cell->setText(label);
            cell->setDetailText(std::to_string(selectedCount) + " Pokémon");
            cell->registerClickAction([=](brls::View*) {
                menuDialog->close([=] {
                    pkdex::PokemonTracker::bulkSetCaptureState(region, selectedDexNumbers, stateIndex, value);
                    refreshRecycler(getFocusedIndexPath());
                });
                return true;
            });
            menuBox->addView(cell);
        };

        addMultiToggle("pkdex/bulk_actions/mark_all_caught"_i18n, 0, true);
        addMultiToggle("pkdex/bulk_actions/mark_all_shiny"_i18n, 1, true);
        if (hasAlpha) {
            addMultiToggle("pkdex/bulk_actions/mark_all_alpha"_i18n, 2, true);
            addMultiToggle("pkdex/bulk_actions/mark_all_shiny_alpha"_i18n, 3, true);
        }

        addMenuSeparator(menuBox);

        addMultiToggle("pkdex/bulk_actions/clear_all_caught"_i18n, 0, false);
        addMultiToggle("pkdex/bulk_actions/clear_all_shiny"_i18n, 1, false);
        if (hasAlpha) {
            addMultiToggle("pkdex/bulk_actions/clear_all_alpha"_i18n, 2, false);
            addMultiToggle("pkdex/bulk_actions/clear_all_shiny_alpha"_i18n, 3, false);
        }

        registerDialogClose(menuDialog);
        menuDialog->open();
        return true;
    }

    // Single-select mode: original behavior
    int actualIndex = currentSelection.section * 30 + currentSelection.row;
    Pokemon& pokemon = s_pokemons[actualIndex];
    pkdex::CaptureStates states = pkdex::PokemonTracker::getCaptureStates(currentRegion, pokemon.regionalDexNumber);

    auto [menuBox, menuDialog] = createMenuDialog("pkdex/capture_toggle/title"_i18n);

    auto addToggle = [&](const std::string& label, bool value, int stateIndex) {
        auto* cell = new brls::BooleanCell();
        cell->init(label, value, [=](bool checked) {
            pkdex::PokemonTracker::toggleCaptureState(currentRegion, pokemon.regionalDexNumber, stateIndex);
            menuDialog->close([=] {
                refreshRecycler(currentSelection);
            });
            return true;
        });
        menuBox->addView(cell);
    };

    addToggle("pkdex/capture_toggle/normal"_i18n, states.normal, 0);
    addToggle("pkdex/capture_toggle/shiny"_i18n, states.shiny, 1);
    if (hasAlpha) {
        addToggle("pkdex/capture_toggle/alpha"_i18n, states.alpha, 2);
        addToggle("pkdex/capture_toggle/shiny_alpha"_i18n, states.shinyAlpha, 3);
    }

    registerDialogClose(menuDialog);
    menuDialog->open();
    return true;
}

bool RecyclingListTab::openBulkActionsDialog(brls::View* view)
{
    // Build a list of all regional dex numbers for the current region
    std::vector<std::string> allDexNumbers;
    for (const auto& pokemon : s_pokemons) {
        allDexNumbers.push_back(pokemon.regionalDexNumber);
    }

    int totalCount = allDexNumbers.size();
    std::string region = currentRegion;
    bool hasAlpha = pkdex::regionHasAlpha(region);

    brls::IndexPath currentSelection = getFocusedIndexPath();

    auto [menuBox, menuDialog] = createMenuDialog("pkdex/bulk_actions/title"_i18n);

    // Helper to add a bulk action button that shows a confirmation dialog
    auto addBulkAction = [=](const std::string& label, int stateIndex, bool value) {
        auto* cell = new brls::DetailCell();
        cell->setText(label);
        cell->setDetailText(std::to_string(totalCount) + " Pokémon");
        cell->registerClickAction([=](brls::View*) {
            // Close the bulk actions menu, then show confirmation
            menuDialog->close([=] {
                std::string confirmMsg = pkdex::formatString(
                    "pkdex/bulk_actions/confirm"_i18n, "{count}", std::to_string(totalCount));

                auto* confirmDialog = new brls::Dialog(confirmMsg);
                confirmDialog->setCancelable(true);
                confirmDialog->addButton("pkdex/common/cancel"_i18n, []() {});
                confirmDialog->addButton("pkdex/common/ok"_i18n, [=]() {
                    pkdex::PokemonTracker::bulkSetCaptureState(region, allDexNumbers, stateIndex, value);
                    refreshRecycler(currentSelection);
                });
                confirmDialog->open();
            });
            return true;
        });
        menuBox->addView(cell);
    };

    // "Mark all as" actions
    addBulkAction("pkdex/bulk_actions/mark_all_caught"_i18n, 0, true);
    addBulkAction("pkdex/bulk_actions/mark_all_shiny"_i18n, 1, true);
    if (hasAlpha) {
        addBulkAction("pkdex/bulk_actions/mark_all_alpha"_i18n, 2, true);
        addBulkAction("pkdex/bulk_actions/mark_all_shiny_alpha"_i18n, 3, true);
    }

    addMenuSeparator(menuBox);

    // "Clear all" actions
    addBulkAction("pkdex/bulk_actions/clear_all_caught"_i18n, 0, false);
    addBulkAction("pkdex/bulk_actions/clear_all_shiny"_i18n, 1, false);
    if (hasAlpha) {
        addBulkAction("pkdex/bulk_actions/clear_all_alpha"_i18n, 2, false);
        addBulkAction("pkdex/bulk_actions/clear_all_shiny_alpha"_i18n, 3, false);
    }

    registerDialogClose(menuDialog);
    menuDialog->open();
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
