
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}
#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
	this->requests = cpy.requests;
	this->serverInfo = cpy.serverInfo;
	this->envp = cpy.envp;
	this->pollPos = cpy.pollPos;
	this->clientSocket = cpy.clientSocket;
	this->body = cpy.body;
	this->clientAddrLen = cpy.clientAddrLen;
	this->serverPoll = cpy.serverPoll;
	this->clientAddr = cpy.clientAddr;
	this->maxBodySizeExeeded = cpy.maxBodySizeExeeded;
	return *this;
}

Socket::Socket(const Socket &cpy)
{
	*this = cpy;
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
		this->pollPos++;
	}
}

void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			this->requests.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->rhMap.erase(this->pollfds[i].fd);
			this->pollPos--;
		}
	}
}

uint32_t	ft_inet_addr(std::string ip)
{
	std::stringstream 	ss(ip);
	std::string			dataBuffer;
	int					ipAddr[4];
	int					i = 0;

	while (getline(ss, dataBuffer, '.'))
	{
		ipAddr[i++] = atoi(dataBuffer.c_str());
	}
	return (htonl((0 | (ipAddr[0] << 24)) | (ipAddr[1] << 16) | (ipAddr[2] << 8) | ipAddr[3]));
}

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->maxBodySizeExeeded = 0;
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	hostValue = this->serverInfo.getHost() != "null" ? this->serverInfo.getHost() : "127.0.0.1";
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			std::cerr << RED << "Error socket init.." << END << std::endl;
			exit(1);
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = ft_inet_addr(hostValue);
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			std::cerr << RED << "error binding socket..." << END << std::endl;
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			std::cerr << RED << "error listening socket..." << END << std::endl;	
			close(serverSocket);
			exit(1);
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 1000;

	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    int bReads = recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	std::cout << "bites read" << bReads << std::endl;
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs())
	{
		std::cout << getContentLenght(this->requests[this->pollfds[i].fd]) << std::endl;
		toRead = getContentLenght(this->requests[this->pollfds[i].fd]);
	}
	else
		toRead = 1000;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && getContentLenght(this->requests[this->pollfds[i].fd]) > (unsigned long)this->serverInfo.getIntMbs())
	{
		this->maxBodySizeExeeded = 1;
		delete [] buffer;
		this->pollfds[i].events = POLLOUT;
		return ;
	}
	else
		this->maxBodySizeExeeded = 0;
	if ((strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && this->requests[this->pollfds[i].fd].substr(this->requests[this->pollfds[i].fd].find("\r\n\r\n") + 4).length() == getContentLenght(this->requests[this->pollfds[i].fd])) || 
		(strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && !getContentLenght(this->requests[this->pollfds[i].fd])))
		this->pollfds[i].events = POLLOUT;
	std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	delete[] buffer;
}

void	Socket::polloutFunc(int i, int debug)
{
	if (this->maxBodySizeExeeded)
	{
		std::string response = atachStatus("HTTP/1.1 413 Request Entity Too Large", fileToStr("./view/displayError/err.html").c_str());
		
		send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
		return ;
	}
	std::string response = findMethod(this->requests[this->pollfds[i].fd], this->serverInfo, this->envp);

	if (debug)
		std::cout << this->requests[this->pollfds[i].fd] << std::endl;
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(int debug)
{
	removePollFds();
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (long unsigned int i = 0 ; i < this->serverInfo.getPorts().size(); i++)
		{
			if (this->pollfds[i].revents & POLLIN)
			{
				this->clientSocket = accept(this->pollfds[i].fd, (struct sockaddr *) &this->clientAddr, &this->clientAddrLen);
				if (this->clientSocket != -1)
				{
					struct pollfd newPoll;
					newPoll.fd = this->clientSocket;
					newPoll.events = POLLIN;
					newPoll.revents = 0;
					addPollFds(newPoll);
				}
			}
		}
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->pollinFunc(i);
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
		}
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
}