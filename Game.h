#pragma once

#include <vector>
#include <string>

using namespace std;

class Game
{
public:
	int getStatus();
	int isOnline();
	string findIp();


private:
	vector<int> online;
	int players = -1;
};

