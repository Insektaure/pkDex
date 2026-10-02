#pragma once

struct Job;

// The high-resolution sprite pack: a zip in the latest release, downloaded to
// the root of the SD card and extracted there (it holds switch/pkDex/resources/).
// Both run on a worker and report through the job.
namespace pack {

constexpr const char* ZIP = "sdmc:/pkDex_High_Res_imgs.zip";

bool zipExists();
void download(Job& job);   // cancellable through job.cancel
void extract(Job& job);

} // namespace pack
