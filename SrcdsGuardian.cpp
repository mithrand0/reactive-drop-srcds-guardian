// SrcdsGuardian.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <vector>
#include <numeric>

#include <Windows.h>
#include <stdio.h>
#include <direct.h>
#include "Shlwapi.h"
#include "psapi.h"

#include "version.h"

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "Shlwapi.lib")

using std::endl;
using std::cout;

static PROCESS_INFORMATION pi = {0};
static STARTUPINFO si = {0};

static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
static int numProcessors;

static std::vector<int> load; // load sampling

void kill_srcds() {
    cout << "! KILLING srcds !" << endl;
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, false, pi.dwProcessId);
    TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
}

void init_cpu() {
    SYSTEM_INFO sysInfo;
    FILETIME ftime, fsys, fuser;

    GetSystemInfo(&sysInfo);
    numProcessors = sysInfo.dwNumberOfProcessors;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&lastCPU, &ftime, sizeof(FILETIME));

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pi.dwProcessId);

    GetProcessTimes(hProcess, &ftime, &ftime, &fsys, &fuser);
    memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
    memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));
}

double get_cpu_usage() {
    FILETIME ftime, fsys, fuser;
    ULARGE_INTEGER now, sys, user;
    double percent;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&now, &ftime, sizeof(FILETIME));

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pi.dwProcessId);

    GetProcessTimes(hProcess, &ftime, &ftime, &fsys, &fuser);
    memcpy(&sys, &fsys, sizeof(FILETIME));
    memcpy(&user, &fuser, sizeof(FILETIME));
    percent = (sys.QuadPart - lastSysCPU.QuadPart) +
        (user.QuadPart - lastUserCPU.QuadPart);
    percent /= (now.QuadPart - lastCPU.QuadPart);
    //percent /= numProcessors;
    lastCPU = now;
    lastUserCPU = user;
    lastSysCPU = sys;

    return percent * 100;
}

void monitor()
{
    Sleep(5000);

    DWORD dwPid = pi.dwProcessId;

    if (dwPid > 0) {
        cout << "+ monitoring srcds.exe: ";
        cout << "pid [" << dwPid << "]";
    
        // memory
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pi.dwProcessId);
        PROCESS_MEMORY_COUNTERS_EX pmc;
        GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
        SIZE_T used = pmc.PrivateUsage;
        int usedMB = ceil(used / 1024 / 1024);

        cout << ", memory [" << usedMB << " MB]";

        // if too high, kill
        if (usedMB > 1023) {
            cout << endl << "MEMORY USAGE TOO HIGH" << endl;
            kill_srcds();
        }

        const double pct = get_cpu_usage();
        cout << ", cpu [" << ceil(pct) << "%]";

        constexpr int samples = 20;
        if (load.size() > samples) load.erase(load.begin());
        if (pct > 0) load.emplace_back(ceil(pct));

        const int avg = ceil(std::accumulate(load.begin(), load.end(), 0) / load.size());

        cout << ", load [" << avg << "%]" << endl;

        if (load.size() > samples && avg > 98) {
            cout << endl << "PROCESS IS BURNING CPU" << endl;
            kill_srcds();
        }
    }
    
    std::thread th = std::thread(monitor);
    th.detach();
}

int main(int argc, char** argv)
{
    // start
    cout << "Initializing Srcds-Guardian .." << endl;
    cout << "Version: " << VERSION << "/" << RELEASE << endl;

    // find -appid
    int appid = 0;
    std::string cmdline;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i - 1], "-appid") == 0) {
            appid = atoi(argv[i]);
        }
        cmdline = cmdline + " " + argv[i];
    }

    cout << endl;

    // if not enough arguments
    if (appid == 0) {
        cout << endl;
        cout << "This program will manage your srcds server. To get started, it requires some arguments." << endl;
        cout << endl;
        cout << "The parameter are the same as srcds.exe, on top of that, you need to specify the appid you want to run." << endl;
        cout << "If you do not specify any other parameter than appid, it will assume sane defaults." << endl;
        cout << endl;
        cout << "A list of appids can be found here     : https://developer.valvesoftware.com/wiki/Dedicated_Servers_List" << endl;
        cout << "A list of parameters can be found here : https://developer.valvesoftware.com/wiki/Command_line_options#Source_2_Games" << endl;
        cout << endl;
        cout << "SrcdsGuardian.exe -appid <appid> [srcds parameters]" << endl;
        cout << "SrcdsGuardian.exe -appid 582400 -game reactivedrop -maxplayers 8 -port 27050 +hostname \"My first Reactive Drop Server\"" << endl;
        cout << endl;

        exit(1);
    }

    // check if we have steamcmd in the working directory
    cout << "Checking for SteamCMD.." << endl;

    LPCWSTR file = L"steamcmd\\steamcmd.exe";

    if (!PathFileExists(file)) {
        cout << "SteamCMD cannot be found, downloading.." << endl;

        std::wstring url = L"https://steamcdn-a.akamaihd.net/client/installer/steamcmd.zip";
        LPWSTR out = const_cast<LPWSTR>(L"steamcmd.zip");

        URLDownloadToFile(NULL, url.c_str(), out, 0, NULL);
        cout << "Download finished, extracting.." << endl;
        system("powershell.exe -Command Expand-Archive -Path steamcmd.zip -Force");
    }

    const int result = _chdir("steamcmd");

    cout << "Starting monitor..";
    std::thread th = std::thread(monitor);
    th.detach();

    cout << "Entering loop..";

    // enter update loop
    while (true) {

        cout << "Updating game.." << endl;

        // install using appid
        const std::string sAppId = std::to_string(appid);
        const std::string cmdUpdate("steamcmd.exe +force_install_dir " + sAppId + " +login anonymous +app_update " + sAppId + " +quit");
        cout << "Executing: " + cmdUpdate << endl;
        system(cmdUpdate.c_str());

        // start the game
        const std::string cmdServer(sAppId + "\\srcds.exe -console " + cmdline);
        cout << "Executing: " + cmdServer << endl;

        size_t len;
        wchar_t wCmd[_MAX_ENV + 1];
        mbstowcs_s(&len, wCmd, _MAX_ENV + 1, cmdServer.c_str(), _MAX_ENV);
            
        if (CreateProcess(NULL, wCmd,
            NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, INFINITE);

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            init_cpu();

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

        cout << "Server did exit, restarting.. " << endl;

        Sleep(10000);
    }
}