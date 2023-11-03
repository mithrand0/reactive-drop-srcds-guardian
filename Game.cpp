#include "Game.h"
#include "Socket.h"
#include <numeric>
#include <string>
#include <Windows.h>
#include <iphlpapi.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

using namespace std;

int Game::getStatus() {

    // query
    const auto server = make_unique<ServerInfo>();

    std::string ip = findIp();
    std::string port = "27015";

    server->start_query(ip, port, "");

    string challenge = server->GetChallenge();
    if (challenge.length()) {
        // retry query with challenge
        server->start_query(ip, port, challenge);
    }

    // read response
    cout << " APPID: " << server->GetAppId() << endl;
    const int maxPlayers = server->GetMaxPlayers();
    const int response = maxPlayers > 0 ? 1 : 0;
    players = server->GetCurPlayers();

    // push a sample
    constexpr int samples = 20;
    if (online.size() > samples) online.erase(online.begin());
    if (samples > 0) online.emplace_back(response);

    cout << "players [" << players << "/" << maxPlayers << "]" << endl;

    return response;
}

int Game::isOnline() {
    
    getStatus();

    const int total = std::accumulate(online.begin(), online.end(), 0);

    return total;
}

string Game::findIp() {

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
        return NULL;
    }
    adapter_addresses = (PIP_ADAPTER_ADDRESSES)malloc(size);

    rv = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, adapter_addresses, &size);
    if (rv != ERROR_SUCCESS) {
        fprintf(stderr, "GetAdaptersAddresses() failed...");
        free(adapter_addresses);
        return NULL;
    }

    for (aa = adapter_addresses; aa != NULL; aa = aa->Next) {
        for (ua = aa->FirstUnicastAddress; ua != NULL; ua = ua->Next) {
            char buf[BUFSIZ];

            int family = ua->Address.lpSockaddr->sa_family;
            //printf("\t%s ", family == AF_INET ? "IPv4" : "IPv6");
            if (family == AF_INET) {
                memset(buf, 0, BUFSIZ);
                getnameinfo(ua->Address.lpSockaddr, ua->Address.iSockaddrLength, buf, sizeof(buf), NULL, 0, NI_NUMERICHOST);

                const string addr(buf);

                if (strcmp("127.", addr.substr(0, 4).c_str()) != 0 && strcmp("169.254.", addr.substr(0, 8).c_str()) != 0) {
                    // non local and no discovery ip
                    return addr;
                }
            }

        }
    }

    free(adapter_addresses);

    return "127.0.0.1";
}
