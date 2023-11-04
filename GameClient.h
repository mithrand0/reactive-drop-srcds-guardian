#pragma once

#include <string>
#include <vector>

using namespace std;

class GameClient
{
public: 
	int isOnline();
	int getStatus();
	vector<string> findIps();
private:
	vector<int> online;
};

