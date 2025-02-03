#include <iostream>
#include <direct.h>
#include <string>
#include <format>
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

        if (!PathFileExists(file)) {
            cout << "Error: download blocked or unzip failed for SteamCmd, please download it manually from:" << endl;
            cout << "https://steamcdn-a.akamaihd.net/client/installer/steamcmd.zip and unzip it to steamcmd/steamcmd.exe" << endl;
            exit(8);
        }
    }
}

void SteamCmd::chdir() {
    const int result = _chdir("steamcmd");
    
    if (result > 0) {
        cout << "Error opening steamcmd directory, exiting." << endl;
        exit(2);
    }
}

void SteamCmd::updateGame(int appid, string branch, bool validate) {
    cout << "Updating game.." << endl << endl;

    // target folder
    std::string folder = std::to_string(appid);

    // if beta given
    std::string beta = "";
    if (strcmp(branch.c_str(), "none") != 0) {
        beta = "-beta " + branch;
    }

    // verify files
    std::string verify = validate ? "validate" : "";

    // as:rd workaround
    std::string prefix = "";

    if (appid == 582400) {
        prefix = appid == 582400 ? "1007 +app_update " : "";
        appid = 563560;
    }

    // install using appid
    const string cmdUpdate(format("steamcmd.exe -overrideminos +force_install_dir {0} +login anonymous +app_update {1} {2} {3} {4} +quit", folder, prefix, appid, beta, verify));
    cout << "Executing: " << cmdUpdate << endl;

    system(cmdUpdate.c_str());
}

void SteamCmd::startGame(int appid, string cmdline, int gamePort) {
    // start the game
    const string cmdServer(format("{}\\srcds.exe -console -nocrashdialog -nomessagebox {}", appid, cmdline));
    cout << "Executing: " << cmdServer << endl;

    size_t len;
    wchar_t wCmd[_MAX_ENV + 1];
    mbstowcs_s(&len, wCmd, _MAX_ENV + 1, cmdServer.c_str(), _MAX_ENV);

    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_MINIMIZE;

    if (CreateProcess(NULL, wCmd,
        NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {

        stats->setPid(pi.dwProcessId);
        port = gamePort;

        // priority
        cout << "Setting priority class.." << endl;
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_INFORMATION, false, pi.dwProcessId);
        SetPriorityClass(hProcess, ABOVE_NORMAL_PRIORITY_CLASS);
        CloseHandle(hProcess);

        // wait until exit
        WaitForSingleObject(pi.hProcess, INFINITE);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        // reset cpu stats
        stats->reset();
        stats->setPid(0);
    }
    else {
        cout << "Start failed!" << endl;
        cout << "Error: " << std::to_string(GetLastError()) << endl;
    }
}

void SteamCmd::cleanUp(int appid) {
    cout << "Cleanup old crashdumps.." << endl;
    
    const string cmdCleanupCrashdumps(format("powershell.exe -Command \"Get-ChildItem -Path {} -File -Recurse -Include '*.mdmp' | Where{{$_.CreationTime -lt (Get-Date).AddDays(-7)}} | Remove-Item -Force\"", appid));
    system(cmdCleanupCrashdumps.c_str());

    cout << "Cleanup old steam cache.." << endl;
    const string cmdCleanupSteamCache(format("powershell.exe -Command \"Get-ChildItem -Path {}/steamapps -File -Recurse -Include '*.vpk' | Where{{$_.CreationTime -lt (Get-Date).AddDays(-7)}} | Remove-Item -Force\"", appid));
    system(cmdCleanupSteamCache.c_str());
}

int SteamCmd::getPid() {
    return pi.dwProcessId;
}

void SteamCmd::killProcess(string reason) {
    cout << endl << "_________________" << endl;
    cout << "! " << reason << endl;
    cout << endl;

    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, false, pi.dwProcessId);
    TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
}

void SteamCmd::checkServer() {
   
    const int pid = getPid();
    
    if (pid > 0) {
        const int memory = (int)ceil(stats->getMemory() / 1024 / 1024);
        const int cpu = stats->getCpu();
        const int load = stats->getLoad();
        const int memorySelf = (int)ceil(stats->getMemorySelf() / 1024);

        if (memory > 1023) killProcess("Killing server: MEMORY TOO HIGH");
        if (load > 98 && stats->getNumSamples() > 20) killProcess("Killing server: SRCDS BURNING CPU");

        const int online = game->isOnline(port);

        const std::string msg(format("monitor: pid [{}], players [{}/{}], memory [{} MB], cpu [{}%], load [{}%]",
            pid, game->GetCurPlayers(), game->GetMaxPlayers(), memory, cpu, load));

        cout << msg << endl;
    }
}

void SteamCmd::initStats() {
    stats = make_unique<Stats>();
    game = make_unique<GameClient>();
}
