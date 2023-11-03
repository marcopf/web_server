#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <iostream>
#include <sstream>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <vector>
#include <map>
#include "../parser/ServerConf.hpp"

#define	MAX_CONN 100

class Socket
{
private:
	struct pollfd					pollfds[MAX_CONN];
    std::map<int, std::string>      requests;
    ServerConf						serverInfo;
    std::vector<std::string>		envp;
	int								pollPos;
    int								clientSocket;
	std::string						body;
    socklen_t						clientAddrLen;
	struct pollfd 					serverPoll;
    struct sockaddr_in				clientAddr;
public:
    void                            polloutFunc(int i);
    void                            pollinFunc(int i);
	void							addPollFds(struct pollfd newPoll);
	void							removePollFds();
    void                            checkFd(void);
    Socket(void);
    Socket(ServerConf server, char **envp);
    ~Socket();
};

#endif