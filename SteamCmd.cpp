#include <iostream>
#include <direct.h>
#include <string>
#include <Windows.h>

#include "SteamCmd.h"
#include "Shlwapi.h"

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "Shlwapi.lib")

using namespace std;

void SteamCmd::install() {
    // check if we have steamcmd in the working directory
    cout << "Checking for SteamCMD.." << endl;

    LPCWSTR file = L"steamcmd\\steamcmd.exe";

    if (!PathFileExists(file)) {
        cout << "SteamCMD cannot be found, downloading.." << endl;

        std::wstring url = L"https://steamcdn-a.akamaihd.net/client/installer/steamcmd.zip";
        LPWSTR out = const_cast<LPWSTR>(L"steamcmd.zip");

        URLDownloadToFile(NULL, url.c_str(), out, 0, NULL);
        cout << "Download finished, extracting.." << endl;
        system("powershell.exe -Command \"Expand-Archive -Path steamcmd.zip -Force\"");
    }
}

void SteamCmd::chdir() {
    const int result = _chdir("steamcmd");
    
    if (result > 0) {
        cout << "Error opening steamcmd directory, exiting." << endl;
        exit(1);
    }
}

void SteamCmd::updateGame(int appid) {
    cout << "Updating game.." << endl;

    // install using appid
    const string sAppId = to_string(appid);
    const string cmdUpdate("steamcmd.exe +force_install_dir " + sAppId + " +login anonymous +app_update " + sAppId + " +quit");
    cout << "Executing: " + cmdUpdate << endl;
    system(cmdUpdate.c_str());
}

void SteamCmd::startGame(int appid, string cmdline) {
    // start the game
    const string cmdServer(to_string(appid) + "\\srcds.exe -console " + cmdline);
    cout << "Executing: " + cmdServer << endl;

    size_t len;
    wchar_t wCmd[_MAX_ENV + 1];
    mbstowcs_s(&len, wCmd, _MAX_ENV + 1, cmdServer.c_str(), _MAX_ENV);

    if (CreateProcess(NULL, wCmd,
        NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {

        stats->setPid(pi.dwProcessId);
        WaitForSingleObject(pi.hProcess, INFINITE);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        // reset cpu stats
        stats->reset();

        // priority
        cout << "Setting priority class.." << endl;
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_INFORMATION, false, pi.dwProcessId);
        SetPriorityClass(hProcess, ABOVE_NORMAL_PRIORITY_CLASS);
        CloseHandle(hProcess);
    }
    else {
        cout << "Start failed!" << endl;
        cout << "Error: " << std::to_string(GetLastError()) << endl;
    }
}

void SteamCmd::cleanUp() {
    cout << "Cleanup old crashdumps.." << endl;
    system("powershell.exe -Command \"Get-ChildItem -File -Recurse -Include '*.mdmp' | Where {`$_.CreationTime -lt (Get-Date).AddDays(-7)} | Remove-Item -Force\"");
}

int SteamCmd::getPid() {
    return pi.dwProcessId;
}

void SteamCmd::killProcess(string reason) {
    cout << reason << endl;
    cout << "! KILLING srcds !" << endl;
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, false, pi.dwProcessId);
    TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
}

void SteamCmd::checkServer() {
   
    const int pid = getPid();
    
    if (pid > 0) {
        const int memory = ceil(stats->getMemory() / 1024 / 1024);
        const int cpu = stats->getCpu();
        const int load = stats->getLoad();

        cout << "monitor: pid [" << pid << "], memory [" << memory << " MB], cpu [" << cpu << "%], load: [" << load << "%]" << endl;

        if (memory > 1023) killProcess("MEMORY TOO HIGH");
        if (load > 98 && stats->getNumSamples() > 20) killProcess("SRCDS BURNING CPU");
    }
}

void SteamCmd::initStats() {
    stats = make_unique<Stats>();
}