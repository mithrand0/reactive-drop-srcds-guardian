// SrcdsGuardian.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <format>
#include <Windows.h>

#include "SteamCmd.h"
#include "version.h"


using namespace std;

unique_ptr<SteamCmd> steamcmd;

void monitor()
{
    Sleep(5000);

    steamcmd->checkServer();
    
    thread th = thread(monitor);
    th.detach();
}

int main(int argc, char** argv)
{
    // start
    cout << "Initializing Srcds-Guardian .." << endl;
    cout << format("Version: {} ({} {})", VERSION, __DATE__, __TIME__) << endl;

    // find -appid
    int appid = 0;

    std::string branch = "none"; // public
    std::string cmdline;
    int port = 27015;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i - 1], "-appid") == 0) appid = atoi(argv[i]);
        if (strcmp(argv[i - 1], "-beta") == 0) branch = argv[i];
        if (strcmp(argv[i - 1], "-port") == 0) port = atoi(argv[i]);
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
        cout << "SrcdsGuardian.exe -appid 582400 -branch beta -game reactivedrop -maxplayers 8 -port 27050 +hostname \"My first Reactive Drop Server\"" << endl;
        cout << endl;

        exit(2);
    }

    // check steamcmd installation
    steamcmd = make_unique<SteamCmd>();

    steamcmd->install();
    steamcmd->chdir();
    
    steamcmd->cleanUp(appid);
    steamcmd->updateGame(appid, branch);

    cout << "Starting monitor.." << endl;

    steamcmd->initStats();
    thread th = thread(monitor);
    th.detach();

    cout << "Entering startup loop.." << endl;

    // enter update loop
    while (true) {

        steamcmd->startGame(appid, cmdline, port);

        cout << "Server did exit." << endl;

        steamcmd->cleanUp(appid);
        
        cout << "Restarting server.." << endl;
        Sleep(10000);

        steamcmd->updateGame(appid, branch);
    }
}