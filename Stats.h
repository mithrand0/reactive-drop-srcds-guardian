#include <Windows.h>
#include <vector>
#include "Shlwapi.h"
#include "psapi.h"

#pragma once
class Stats
{
public:
	void initStats();
	void setPid(int pid);
	void reset();
	int getMemory();
	int getMemorySelf();
	int getCpu();
	int getLoad();
	int getNumSamples();

private:
	ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
	int pid;
	int numProcessors;
	std::vector<int> load; // load sampling
};

