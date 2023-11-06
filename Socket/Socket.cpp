#include "Socket.hpp"
#include "RequestHelper.hpp"

Socket::Socket()
{
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		this->requests.insert(std::pair<int, std::string>(newPoll.fd, ""));
		// this->rhMap.insert(std::pair<int, RequestHandler>(newPoll.fd, RequestHandler()));
	}
	this->pollPos++;
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

Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	
	while (envp_main[++i])
		this->envp.push_back(envp_main[i]);
	this->pollPos = 0;
	this->body = "";
	this->serverInfo = data;
	this->clientAddrLen = sizeof(clientAddr);
	for (long unsigned int i = 0; i < this->serverInfo.getPorts().size(); i++)
	{
		struct sockaddr_in	serverAddr;
		serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		portsOption[i] = 1;
		setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &portsOption, sizeof(portsOption[i]));
		if (serverSocket == -1)
		{
			perror("Error socket init..");
			return ;
		}
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(this->serverInfo.getPorts()[i]);
		serverAddr.sin_addr.s_addr = INADDR_ANY;
		if (bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) == -1)
		{
			perror("error binding socket...");
			close(serverSocket);
			return ;
		}
		if (listen(serverSocket, 5) == -1)
		{
			perror("error listening socket...");
			close(serverSocket);
			return ;
		}
		memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN | POLLOUT;
		std::cout << this->pollPos << std::endl;
		addPollFds(this->serverPoll);
		std::cout << this->pollPos << std::endl;
		std::cout << "Listening on port: " << this->serverInfo.getPorts()[i] << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char					*buffer;
	static unsigned long	toRead = 50;

	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n"))
		this->pollfds[i].events = POLLOUT;
	buffer = new char[toRead + 1];
	memset(buffer, 0, toRead + 1);
    recv(this->pollfds[i].fd, buffer, toRead, MSG_DONTWAIT);
	this->requests[this->pollfds[i].fd] += buffer;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n")
		&& RequestHelper::getContentLenght(this->requests[this->pollfds[i].fd]) <= (unsigned long)this->serverInfo.getIntMbs()	)
		toRead = RequestHelper::getContentLenght(this->requests[this->pollfds[i].fd]);
	else
		toRead = 50;
	if (strstr(this->requests[this->pollfds[i].fd].c_str(), "\r\n\r\n") && 
		!RequestHelper::getContentLenght(this->requests[this->pollfds[i].fd]))
		this->pollfds[i].events = POLLOUT;
	delete[] buffer;
}

void	Socket::polloutFunc(int i)
{
	// this->rhMap[this->pollfds[i].fd].findMethodAndUrl();
	// this->rhMap[this->pollfds[i].fd].checkLocation();RequestHelper
	// std::string res = this->rhMap[this->pollfds[i].fd].getResponse();
	std::string response = RequestHelper::findMethod(this->requests[this->pollfds[i].fd], this->serverInfo);
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(void)
{
	removePollFds();
	if (poll(this->pollfds, this->pollPos, 500) > 0)
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
			{
				this->pollinFunc(i);
				return ;
			}
			if (this->pollfds[i].revents == POLLOUT)
			{
				this->polloutFunc(i);
				return ;
			}
		}
	}
}

Socket::~Socket()
{
}