#include <Windows.h>
#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include "global.hpp"
#include "downloader.hpp"
#include "CS2Patch.hpp"

int main(int argc, char* argv[]) {
	std::string wantsMovementPatch;

	if (Downloader::needsUpdate()) {
		puts("update required, press enter to download new update.");
		waitforinput();
		Downloader::UpdateInstaller();
	}

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "disablemanifest") == 0) {
			Globals::usesNoManifests = true;
			break;
		}
	}
	puts("Daived: Thx for use my app , Preparing Download...");
	Downloader::PrepareDownload();
	puts("Prepared Download!");
	Patcher::CleanPatchFiles();
	puts("Starting Download...");
	Downloader::DownloadCS2();
	puts("Finished Download!.");
	puts("Starting Patches...");
	Patcher::PatchClient();
	puts("Do you want to install the Patch?");
	puts("Press 'Y' for 'Yes' or 'N' for 'No' on your keyboard");
	std::cin >> wantsMovementPatch;

	for (char& c : wantsMovementPatch) { 
		c = std::tolower(c);
	}

	if (wantsMovementPatch.find("y") != std::string::npos) {
		Patcher::PatchServer();
	}

	puts("Finished Patches!");
	puts("Starting GamePatches...");
	puts("This can take a long time...");
	Downloader::DownloadMods();
	puts("Finished GamePatches!");
	puts("Cleaning up downloader files...");
	if (!Globals::usesNoManifests) {
		std::filesystem::remove_all("manifestFiles");
	}
	puts("Cleaned up!");
	puts("Download complete!");
	puts("Daived: Thanks for use my app, Press 'Enter' to close the installer!");
	waitforinput();
	return 0;
}
