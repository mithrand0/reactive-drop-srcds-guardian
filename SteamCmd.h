#pragma once

#include <string>
#include <memory>

#include "Stats.h"
#include "GameClient.h"

using namespace std;

class SteamCmd
{
public:
	void initStats();
	void install();
	void chdir();
	void updateGame(int appid, string branch);
	void startGame(int appid, string cmdline, int port);
	void cleanUp();
	int getPid();
	void killProcess(string reason);
	void checkServer();
private:
	int port;

	unique_ptr<Stats> stats;
	unique_ptr<GameClient> game;

	PROCESS_INFORMATION pi = {0};
	STARTUPINFO si = {0};
};

