#define _WINSOCKAPI_

#include "UdpClient.h"

#include <numeric>
#include <string>
#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <memory>
#include <iostream>

#include "GameClient.h"

using namespace std;

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")


int GameClient::getStatus(int gamePort) {

    // initialize address list on first load
    if (addrlist.size() < 1) addrlist = findIps();

    // query
    static std::unique_ptr<UdpClient> client;

    if (!client) {
        client = make_unique<UdpClient>();
    }

    string port(to_string(gamePort));

    int response = -1;
    string addr;

    for (string entry : addrlist) {
        addr = entry;
        if (response != 0 && response != -4)
            client->query(addr, port);
    }

    // if no answer, repeat the query on the last interface
    // it should have the challenge stored internally
    if (client->GetMaxPlayers() < 1)
        client->query(addr, port);

    // if it is still not responding, it's offline
    const bool responding = client->GetMaxPlayers() > 0;

    // store this for stats
    curPlayers = client->GetCurPlayers();
    maxPlayers = client->GetMaxPlayers();
    
    // push a sample
    constexpr int samples = 20;
    if (online.size() > samples) online.erase(online.begin());
    if (samples > 0) online.emplace_back(responding);

    return responding;
}

int GameClient::isOnline(int port) {

    getStatus(port);
    const int total = std::accumulate(online.begin(), online.end(), 0);

    return total;
}

int GameClient::GetCurPlayers() {
    return curPlayers;
}

int GameClient::GetMaxPlayers() {
    return maxPlayers;
}

vector<string> GameClient::findIps() {

    //Initialise winsock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Failed. Error Code : %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }

    DWORD rv, size;
    PIP_ADAPTER_ADDRESSES adapter_addresses, aa;
    PIP_ADAPTER_UNICAST_ADDRESS ua;

    rv = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, NULL, &size);
    if (rv != ERROR_BUFFER_OVERFLOW) {
        fprintf(stderr, "GetAdaptersAddresses() failed...");
        return {};
    }
    adapter_addresses = (PIP_ADAPTER_ADDRESSES)malloc(size);

    rv = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, adapter_addresses, &size);
    if (rv != ERROR_SUCCESS) {
        fprintf(stderr, "GetAdaptersAddresses() failed...");
        free(adapter_addresses);
        {
            {
                return {};
            }
        }
    }

    vector<std::string> addrlist = {};

    for (aa = adapter_addresses; aa != NULL; aa = aa->Next) {
        for (ua = aa->FirstUnicastAddress; ua != NULL; ua = ua->Next) {
            char buf[BUFSIZ];

            int family = ua->Address.lpSockaddr->sa_family;
            //printf("\t%s ", family == AF_INET ? "IPv4" : "IPv6");
            if (family == AF_INET) {
                memset(buf, 0, BUFSIZ);
                getnameinfo(ua->Address.lpSockaddr, ua->Address.iSockaddrLength, buf, sizeof(buf), NULL, 0, NI_NUMERICHOST);

                const std::string addr(buf);

                if (strcmp("127.", addr.substr(0, 4).c_str()) != 0 && strcmp("169.254.", addr.substr(0, 8).c_str()) != 0) {
                    // non local and no discovery ip
                    addrlist.push_back(addr);
                }
            }

        }
    }

    free(adapter_addresses);
    WSACleanup();

    return addrlist;
}
