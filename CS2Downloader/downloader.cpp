#include "downloader.hpp"
#include "global.hpp"
#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <conio.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")

bool DownloadFile(const char* url, const char* outputFile) { 
	HINTERNET hInternet = InternetOpenA("FileDownloader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInternet) {
		puts("failed to initialize wininet.");
		return false;
	}

	HINTERNET hUrl = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
	if (!hUrl) {
		puts("failed to access url.");
		return false;
	}

	HANDLE hFile = CreateFileA(outputFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		printf("failed to download file -> %s\n", outputFile);
		return false;
	}

	BYTE buffer[4096];
	memset(buffer, 0, sizeof(buffer)); 

	DWORD bytesRead;
	while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
		DWORD bytesWritten;
		WriteFile(hFile, buffer, bytesRead, &bytesWritten, NULL);
	}
	CloseHandle(hFile);

	InternetCloseHandle(hUrl);
	InternetCloseHandle(hInternet);
	return true;
}

std::string ReadOnlineString(const char* url) {
	std::string result = "";

	HINTERNET hInternet = InternetOpenA("URLReader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (!hInternet) {
		puts("failed to initialize wininet.");
		return result;
	}

	HINTERNET hUrl = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
	if (!hUrl) {
		puts("failed to access url.");
		return result;
	}

	char buffer[4096];
	memset(buffer, 0, sizeof(buffer)); 

	DWORD bytesRead;
	while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
		result.append(buffer, bytesRead);
	}

	InternetCloseHandle(hUrl);
	InternetCloseHandle(hInternet);
	return result;
}

std::filesystem::path GetLocalAppData() {
	char* localAppData;
	size_t bufferSize = 0;
	errno_t result = _dupenv_s(&localAppData, &bufferSize, "LOCALAPPDATA");
	if (result != NULL || !localAppData) {
		puts("failed to get local appdata directory.");
		waitforinput();
		exit(1);
	}

	std::filesystem::path localAppDataPath(static_cast<const char*>(localAppData));

	return localAppDataPath;
}

bool Downloader::needsUpdate() {
	std::string versionString = ReadOnlineString("https://raw.githubusercontent.com/McDaived/CS2-Patch-Access/main/CS2Downloader/global.hpp");
	if (!versionString.empty() && versionString.find(Globals::currentVersion) == std::string::npos) 
		return true;

	return false;
}

void Downloader::UpdateInstaller() {
	std::string currentAppPath;
	std::string updatedAppPath;


	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	currentAppPath = buffer;
	updatedAppPath = currentAppPath + ".temp";


	if (!DownloadFile("https://github.com/McDaived/CS2-Patch-Access/raw/main/App/CS2Downloader.exe", updatedAppPath.c_str())) {
		puts("failed to download update.");
		waitforinput();
		exit(1);
	}
	std::string currentAppName = currentAppPath.substr(currentAppPath.find_last_of("\\/") + 1);
	std::string updatedAppName = updatedAppPath.substr(updatedAppPath.find_last_of("\\/") + 1);

	std::string deleteCommand = "del \"" + currentAppName + "\" & ren \"" + updatedAppName + "\" \"" + currentAppName + "\" & del \"%~f0\"";
	std::string batchFilePath = currentAppPath + ".bat";
	std::ofstream batchFile(batchFilePath);
	if (batchFile.is_open()) {
		batchFile << "@echo off\n";
		batchFile << deleteCommand;
		batchFile.close();
	}
	else {
		puts("failed to create the deletion script.");
		waitforinput();
		exit(1);
	}

	STARTUPINFOA startupInfo;
	PROCESS_INFORMATION processInfo;
	ZeroMemory(&startupInfo, sizeof(startupInfo));
	ZeroMemory(&processInfo, sizeof(processInfo));
	startupInfo.cb = sizeof(startupInfo);
	if (CreateProcessA(NULL, (LPSTR)batchFilePath.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo)) {
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
	else {
		puts("failed to create the deletion process.");
		waitforinput();
		exit(1);
	}

	exit(0);
}

void Downloader::PrepareDownload() {
	std::filesystem::path currentPath = std::filesystem::current_path();
	std::filesystem::path localAppData = GetLocalAppData();
	const char* manifestNames[3] = { "730_2347770", "730_2347771", "730_2347779" };
	const char* depotKeys = "depot_keys.json";

	if (!Globals::usesNoManifests)
		std::filesystem::remove_all("manifestFiles"); 

	std::string stringPath = (currentPath / "manifestFiles").string();

	std::filesystem::create_directory(currentPath / "manifestFiles");

	if (!Globals::usesNoManifests) {
		int maxIndex = sizeof(manifestNames) / sizeof(manifestNames[0]);
		for (int downloadIndex = 0; downloadIndex < maxIndex; ++downloadIndex) { 
			std::string downloadLink = "https://github.com/McDaived/CS2Downloader-Manifests/raw/main/";
			downloadLink += manifestNames[downloadIndex];

			std::string downloadPath = "manifestFiles/";
			downloadPath += manifestNames[downloadIndex];

			DownloadFile(downloadLink.c_str(), downloadPath.c_str());

			
		}
	}

	if (std::filesystem::exists(localAppData / "steamctl"))
		std::filesystem::remove_all(localAppData / "steamctl");

	Sleep(1000);

	std::filesystem::path steamctlDirectory = localAppData / "steamctl";
	std::filesystem::create_directory(steamctlDirectory);
	std::filesystem::create_directory(steamctlDirectory / "steamctl");

	if (true) {
		std::string downloadLink = "https://github.com/McDaived/CS2Downloader-Manifests/raw/main/";
		downloadLink += depotKeys;

		std::string downloadPath = localAppData.string();
		downloadPath += "\\steamctl\\steamctl\\";
		downloadPath += depotKeys;

		DownloadFile(downloadLink.c_str(), downloadPath.c_str());

	}

	if (!std::filesystem::exists("python-3.11.4-embed-amd64") || !std::filesystem::exists("python-3.11.4-embed-amd64/python.exe")) {
		puts("python not found. make sure the \"python-3.11.4-embed-amd64\" folder exists and contains python.exe.");
		waitforinput();
		exit(1);
	}

}

void Downloader::DownloadCS2() {
	std::filesystem::path currentPath = std::filesystem::current_path();
	const char* manifestNames[3] = { "730_2347770", "730_2347771", "730_2347779" };

	std::string stringPath = currentPath.string();
	std::filesystem::current_path(stringPath.c_str());

	int maxIndex = sizeof(manifestNames) / sizeof(manifestNames[0]);
	for (int downloadIndex = 0; downloadIndex < (maxIndex); ++downloadIndex) {
		std::string manifestPath = "manifestFiles\\";
		manifestPath += manifestNames[downloadIndex];

		std::string executeDownload = "\"" + currentPath.string() + "\\";
		executeDownload += "python-3.11.4-embed-amd64\\python.exe";
		executeDownload += "\"";
		executeDownload += " -m steamctl depot download -f ";
		executeDownload += manifestPath;
		executeDownload += " --skip-licenses --skip-login";
		system(executeDownload.c_str());
	}

	std::ifstream assettypes_common("game/bin/assettypes_common.txt");
	std::ofstream assettypes_internal("game/bin/assettypes_internal.txt");

	assettypes_internal << assettypes_common.rdbuf();

	std::ifstream sdkenginetools("game/bin/sdkenginetools.txt");
	std::ofstream enginetools("game/bin/enginetools.txt");

	enginetools << sdkenginetools.rdbuf();
}

void Downloader::DownloadMods() {
	std::filesystem::path currentPath = std::filesystem::current_path();
	const char* githubPaths[] = {
		"https://github.com/McDaived/CS2-Patch-Access/raw/main/GamePatch/game/csgo_mods/pak01_000.vpk",
		"https://github.com/McDaived/CS2-Patch-Access/raw/main/GamePatch/game/csgo_mods/pak01_dir.vpk",
		"https://raw.githubusercontent.com/McDaived/CS2-Patch-Access/main/GamePatch/game/csgo/gameinfo.gi",
		"https://raw.githubusercontent.com/McDaived/CS2-Patch-Access/main/GamePatch/game/csgo/scripts/vscripts/banList.lua",
		"https://github.com/CS2-Patch-Access/raw/main/GamePatch/Start%20Game%20.bat",
		"https://github.com/CS2-Patch-Access/raw/main/GamePatch/Start%20Server.bat",
		"https://github.com/CS2-Patch-Access/raw/main/GamePatch/Workshop%20Tools.bat" };

	const char* filePaths[] = {
		"game\\csgo_mods\\pak01_000.vpk",
		"game\\csgo_mods\\pak01_dir.vpk",
		"game\\csgo\\gameinfo.gi",
		"game\\csgo\\scripts\\vscripts\\banList.lua",
		"Start Game.bat",
		"Start Server.bat",
		"Workshop Tools.bat" };

	const char* replaceExceptionList[] = {
		"banList.lua"
	};

	std::filesystem::remove_all(currentPath / "game" / "csgo_mods");

	std::filesystem::create_directory(currentPath / "game" / "csgo_mods");
	std::filesystem::create_directory(currentPath / "game" / "csgo" / "scripts");
	std::filesystem::create_directory(currentPath / "game" / "csgo" / "scripts" / "vscripts");
	std::filesystem::create_directory(currentPath / "game" / "bin" / "win64");

	int maxFileIndex = sizeof(filePaths) / sizeof(filePaths[0]);
	for (int fileIndex = 0; fileIndex < maxFileIndex; ++fileIndex) {
		std::filesystem::path filePath = filePaths[fileIndex];
		int maxExceptionIndex = sizeof(replaceExceptionList) / sizeof(replaceExceptionList[0]);
		for (int exceptionIndex = 0; exceptionIndex < maxExceptionIndex; ++exceptionIndex) {
			std::string stringPath = filePaths[fileIndex];
			if (stringPath.find(replaceExceptionList[exceptionIndex]) == std::string::npos) { 
				if (std::filesystem::exists(filePath)) {
					std::filesystem::remove(filePath);
				}
			}
		}

		std::filesystem::path downloadFilePath = currentPath / filePaths[fileIndex];
		std::string downloadPath = downloadFilePath.string();


		DownloadFile(githubPaths[fileIndex], downloadPath.c_str());
	}
}
