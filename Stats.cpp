#include "Stats.h"

#include <numeric>
#include <iostream>

using namespace std;

void Stats::setPid(int processId) {
    pid = processId;
}

void Stats::reset() {
    SYSTEM_INFO sysInfo;
    FILETIME ftime, fsys, fuser;

    GetSystemInfo(&sysInfo);
    numProcessors = sysInfo.dwNumberOfProcessors;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&lastCPU, &ftime, sizeof(FILETIME));

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    GetProcessTimes(hProcess, &ftime, &ftime, &fsys, &fuser);
    CloseHandle(hProcess);

    memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
    memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));
}


int Stats::getCpu() {
    FILETIME ftime, fsys, fuser;
    ULARGE_INTEGER now, sys, user;
    double percent;

    GetSystemTimeAsFileTime(&ftime);
    memcpy(&now, &ftime, sizeof(FILETIME));

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    GetProcessTimes(hProcess, &ftime, &ftime, &fsys, &fuser);
    CloseHandle(hProcess);

    memcpy(&sys, &fsys, sizeof(FILETIME));
    memcpy(&user, &fuser, sizeof(FILETIME));
    percent = (sys.QuadPart - lastSysCPU.QuadPart) +
        (user.QuadPart - lastUserCPU.QuadPart);
    percent /= (now.QuadPart - lastCPU.QuadPart);
    percent *= 100;
    //percent /= numProcessors;
    lastCPU = now;
    lastUserCPU = user;
    lastSysCPU = sys;

    constexpr int samples = 20;
    if (load.size() > samples) load.erase(load.begin());
    if (percent >= 0) load.emplace_back(ceil(percent));

    return percent;
}

int Stats::getLoad() {
    const int avg = ceil(accumulate(load.begin(), load.end(), 0) / load.size());
    return avg > 0 ? avg : 0;
}


int Stats::getMemory() {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    SIZE_T used = pmc.PrivateUsage;

    return used;
}

int Stats::getNumSamples() {
    return load.size();
}