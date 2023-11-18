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
#include "../utils/utils.hpp"
#include "Connection.hpp"

#define	MAX_CONN 300
#define RED "\x1b[1;31m"
#define CYAN "\x1b[1;36m"
#define GREEN "\x1b[1;32m"
#define YELLOW "\x1b[1;33m"
#define END "\x1b[1;0m"

class Socket
{
private:
	struct pollfd					pollfds[MAX_CONN];
	std::map<int, Connection *>		connections;
	std::vector<std::string>		envp;
	ServerConf						serverInfo;
	std::string						body;
	socklen_t						clientAddrLen;
	int								pollPos;
	int								clientSocket;
	int								maxBodySizeExeeded;
	struct pollfd 					serverPoll;
	struct sockaddr_in				clientAddr;
public:
	void							polloutFunc(int i, int debug);
	void							pollinFunc(int i);
	void							addPollFds(struct pollfd newPoll);
	void							removePollFds();
	void							checkFd(int debug);
	Socket &operator=(const Socket   &cpy);
	Socket(const Socket &cpy);
	Socket(void);
	Socket(ServerConf server, char **envp);
	~Socket();
};

#endif