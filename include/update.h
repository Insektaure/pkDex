#pragma once
#include <string>

struct Job;

// Self-update from the GitHub releases, replacing the external pkDexUpdater.
namespace Update {

enum class State {
    Idle,       // no check asked for yet
    Checking,   // asking GitHub
    UpToDate,   // this build is the newest release
    Available,  // a newer release with a pkDex.zip (or pkDex.nro) exists
    Failed,     // no network, or GitHub did not answer
};

// Asks GitHub on a worker thread. Does nothing while a check is running.
void beginCheck();
void shutdown();   // waits for the worker

State state();
std::string latestVersion();   // the release tag, without the leading "v"
std::string lastError();
std::string assetName();       // the file the update comes in, "pkDex.zip"

// The steps install() reports through job.stage, in order.
enum Step { StepDownload, StepUnpack, StepVerify, StepBackup, StepInstall, StepCleanup, StepCount };

// Downloads and installs the release the check found: over `exePath` when it
// is installPath(), to installPath() otherwise (an install from before 2.0.0
// moves there; restart into installPath(), the old copy is deleted on the next
// start by cleanupLegacy()).
// Blocking: run it on a worker. Draw nothing from the RomFS meanwhile: it is
// unmounted during the install step, since it is mounted out of the very file
// being replaced. On failure the message says why, in English, and the
// running build is intact.
void install(const std::string& exePath, Job& job);

// The homebrew loader starts `exePath` when the app exits, instead of the menu.
void restartInto(const std::string& exePath);

std::string backupPath();
std::string installPath();                          // sdmc:/switch/pkDex/pkDex.nro
bool installsInPlace(const std::string& exePath);   // exePath is installPath()

// Dotted versions ("1.2.3", "v1.2"): >0 when `a` is newer than `b`.
int compareVersions(const std::string& a, const std::string& b);

// The DisplayVersion / the (first) title in an NRO's embedded NACP, empty when
// it is not an NRO.
std::string nroDisplayVersion(const std::string& path);
std::string nroTitle(const std::string& path);

// At launch: files the external updater left behind (its own NRO, a
// pkDex.nro.new it never applied) and, running from installPath(), the copy an
// update moved away from and a 1.x /switch/pkDex.nro - each only if its NACP
// says pkDex.
void cleanupLegacy(const std::string& exePath);

} // namespace Update
