# srcds-guardian

Little console utility that wraps around srcds.exe and steamcmd.exe

It provides the following features:

- automatic restarts of servers
- automatic server updates from steam;
- optimizes the resource class for srcds and steamcmd; 
- watchdog:
  * checks for memory leaks, and restarts if needed;
  * checks for servers that are hanging;
  * checks for servers that burn 100% cpu;
  * auto restarts after the last player leaves.

 In short: everything you need to run a hazzle free server.
