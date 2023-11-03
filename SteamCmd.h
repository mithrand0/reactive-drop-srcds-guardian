#pragma once

#include <string>
#include <memory>

#include "Stats.h"
#include "Game.h"

using namespace std;

class SteamCmd
{
public:
	void initStats();
	void install();
	void chdir();
	void updateGame(int appid);
	void startGame(int appid, string cmdline);
	void cleanUp();
	int getPid();
	void killProcess(string reason);
	void checkServer();
private:

	unique_ptr<Stats> stats;
	unique_ptr<Game> game;

	PROCESS_INFORMATION pi = {0};
	STARTUPINFO si = {0};
};

