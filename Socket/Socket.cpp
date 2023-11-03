#include "Socket.hpp"

Socket::Socket()
{
}

std::string atachStatus(const char *status, const char *type, const char *body)
{
	std::stringstream ss;
	std::string status_s = status, body_s = body;
	ss << "\r\nContent-length: " << body_s.length() << "\r\nContent-Type: " << type << "\r\n\r\n";
	std::string res = status_s + ss.str() + body_s + "\r\n";
	return (res);
}

std::string	fileToStr(std::string file)
{
	std::string str;
	std::ifstream f(file.c_str());
	std::ostringstream ss;
	ss << f.rdbuf();
	str = ss.str();
	f.close();
	return (str);
}

void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		// this->fdsMap.insert(std::pair<int, std::string>(newPoll.fd, ""));
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
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			// this->fdsMap.erase(this->pollfds[i].fd);
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
		this->serverPoll.events = POLLIN;
		std::cout << this->pollPos << std::endl;
		addPollFds(this->serverPoll);
		std::cout << this->pollPos << std::endl;
		std::cout << "Listening on port: " << this->serverInfo.getPorts()[i] << std::endl;

	}
}

void	Socket::pollinFunc(int i)
{
	char		buffer[2024];
	std::string	header;

	memset(buffer, 0, sizeof(buffer));
    int j = recv(this->pollfds[i].fd, buffer, sizeof(buffer), MSG_DONTWAIT);
    buffer[j] = 0;
	this->header += buffer;
	std::cout << "Header: " << this->header << std::endl;
    // if (strstr(buffer, "\r\n\r\n"))
	// {
    // 	std::cout << this->header << std::endl;
    this->pollfds[i].events = POLLOUT;
	this->header = "";
	// 	return ;
	// }
	// std::cout << this->header << "hey" << std::endl;
}

void	Socket::polloutFunc(int i)
{

	// this->rhMap[this->pollfds[i].fd].findMethodAndUrl();
	// this->rhMap[this->pollfds[i].fd].checkLocation();
	// std::string res = this->rhMap[this->pollfds[i].fd].getResponse();
	std::string response = atachStatus("HTTP/1.1 200 OK", "text/html", fileToStr("./welcome.html").c_str());
	send(this->pollfds[i].fd, response.c_str(), response.length(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


void	Socket::checkFd(void)
{
	removePollFds();
	std::cout << "pollPos: " << this->pollPos << std::endl;
	if (poll(this->pollfds, this->pollPos, 500) > 0)
	{
		for (int i = 0 ; i < this->pollPos; i ++)
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
		for (int i = 1; i < this->pollPos; i++)
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