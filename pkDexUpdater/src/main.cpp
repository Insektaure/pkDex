#include <switch.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>

// Path constants
const std::string SOURCE_PATH = "/switch/pkDex.nro.new";
const std::string TARGET_PATH = "/switch/pkDex.nro";
const std::string BACKUP_PATH = "/switch/pkDex.nro.bak";

// Function to check if a file exists
bool fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

// Function to rename a file with error handling
bool renameFile(const std::string& source, const std::string& target) {
    if (!fileExists(source)) {
        return false;
    }

    return rename(source.c_str(), target.c_str()) == 0;
}

// Function to delete a file
bool deleteFile(const std::string& path) {
    if (!fileExists(path)) {
        return false;
    }

    return remove(path.c_str()) == 0;
}

// Function to apply the update
bool applyUpdate() {
    bool success = false;
    std::string errorMessage;

    // Print status message
    printf("pkDex Updater\n");
    consoleUpdate(NULL);
    printf("-------------\n");
    consoleUpdate(NULL);
    printf("Applying update...\n");
    consoleUpdate(NULL);

    sleep(3);

    // Check if the new file exists
    if (!fileExists(SOURCE_PATH)) {
        errorMessage = "Update file not found: " + SOURCE_PATH;
    } else {
        printf("Found update file: %s\n", SOURCE_PATH.c_str());
        consoleUpdate(NULL);

        sleep(3);

        // Create a backup of the current file if it exists
        if (fileExists(TARGET_PATH)) {
            printf("Creating backup of current version...\n");
            consoleUpdate(NULL);
            if (!renameFile(TARGET_PATH, BACKUP_PATH)) {
                errorMessage = "Failed to create backup of current version";
            } else {
                printf("Backup created: %s\n", BACKUP_PATH.c_str());
                consoleUpdate(NULL);

                sleep(3);
            }
        }

        // If backup was successful or not needed, proceed with the update
        if (errorMessage.empty()) {
            printf("Installing update...\n");
            consoleUpdate(NULL);
            // Rename the new file to the target path
            if (renameFile(SOURCE_PATH, TARGET_PATH)) {
                success = true;
                printf("Update successfully applied!\n");
                consoleUpdate(NULL);

                sleep(3);

                // Delete the backup file if it exists
                if (fileExists(BACKUP_PATH)) {
                    if (deleteFile(BACKUP_PATH)) {
                        printf("Backup file deleted.\n");
                        consoleUpdate(NULL);
                    } else {
                        printf("\033[0;31mWarning: Failed to delete backup file.\033[0m\n");
                        consoleUpdate(NULL);
                    }

                    sleep(3);
                }
            } else {
                errorMessage = "Failed to apply update";

                // Try to restore from backup if available
                if (fileExists(BACKUP_PATH)) {
                    printf("Attempting to restore from backup...\n");
                    consoleUpdate(NULL);
                    if (renameFile(BACKUP_PATH, TARGET_PATH)) {
                        errorMessage += " (restored from backup)";
                        printf("Restored from backup.\n");
                        consoleUpdate(NULL);
                    } else {
                        errorMessage += " (failed to restore from backup)";
                        printf("\033[0;31mFailed to restore from backup.\033[0m\n");
                        consoleUpdate(NULL);
                    }

                    sleep(3);
                }
            }
        }
    }

    // Print final status
    if (!success) {
        printf("\033[0;31mError: %s\033[0m\n", errorMessage.c_str());
        consoleUpdate(NULL);

        sleep(3);
    }

    return success;
}

int main(int argc, char* argv[]) {
    // Initialize the Switch services
    socketInitializeDefault();
    consoleInit(NULL);

    // Apply the update
    bool success = applyUpdate();

    // Wait a few seconds before exiting
    printf("\nExiting in 5 seconds...\n");
    consoleUpdate(NULL);
    sleep(5);

    // Clean up
    consoleExit(NULL);
    socketExit();

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
