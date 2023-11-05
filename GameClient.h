#pragma once

#include <string>
#include <vector>

using namespace std;

class GameClient
{
public: 
	int isOnline(int port);
	int getStatus(int port);
	vector<string> findIps();

	int GetCurPlayers();
	int GetMaxPlayers();
private:
	vector<int> online;
	int curPlayers = 0;
	int maxPlayers = 0;
};

