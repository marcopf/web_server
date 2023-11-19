#include "Socket.hpp"

Socket	&Socket::operator=(const Socket &cpy)
{
	ft_memcpy(this->pollfds, cpy.pollfds, sizeof(struct pollfd) * MAX_CONN);
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

/**
 * The function adds a new pollfd to the Socket's pollfds array and creates a new Connection object
 * associated with the pollfd.
 * 
 * @param newPoll The parameter `newPoll` is of type `struct pollfd`. It represents a file descriptor
 * that will be added to the `pollfds` array in the `Socket` class.
 */
void	Socket::addPollFds(struct pollfd newPoll)
{
	if (this->pollPos < MAX_CONN)
	{
		this->pollfds[this->pollPos] = newPoll;
		Connection *toAdd = new Connection(&this->pollfds[this->pollPos]);
		this->connections.insert(std::pair<int, Connection *>(newPoll.fd, toAdd));
		this->pollPos++;
	}
}

/**
 * The function removes any poll file descriptors that have encountered an error.
 */
void	Socket::removePollFds()
{
	for (int i = 1; i <= this->pollPos; i++)
	{
		if (this->pollfds[i].revents == POLLERR)
		{
			delete this->connections[this->pollfds[i].fd];
			this->connections.erase(this->pollfds[i].fd);
			close(this->pollfds[i].fd);
			for (int j = i; j < this->pollPos; j++)
				this->pollfds[j] = this->pollfds[j + 1];
			this->pollPos--;
		}
	}
}

/**
 * The function `ft_inet_addr` takes a string representation of an IP address and converts it into a
 * 32-bit integer in network byte order.
 * 
 * @param ip The "ip" parameter is a string that represents an IP address in the format "x.x.x.x",
 * where each "x" is a number between 0 and 255.
 * 
 * @return The function `ft_inet_addr` returns a `uint32_t` value, which is an unsigned 32-bit integer.
 */
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

/**
 * The Socket constructor initializes a server socket and binds it to the specified host and ports.
 * 
 * @param data The `data` parameter is of type `ServerConf` and contains information about the server
 * configuration, such as the host and ports to listen on.
 * @param envp_main `envp_main` is a pointer to an array of strings that represents the environment
 * variables passed to the program.
 * 
 * @return There is no return statement in the code snippet provided, so nothing is being returned.
 */
Socket::Socket(ServerConf data, char **envp_main)
{
	int	serverSocket;
    int portsOption[this->serverInfo.getPorts().size()], i = -1;
	std::string	hostValue;
	
	ft_memset(this->pollfds, 0, sizeof(struct pollfd) * MAX_CONN);
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
			throw(1);
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
			throw(1);
		}
		ft_memset(&this->serverPoll, 0, sizeof(this->serverPoll));
		this->serverPoll.fd = serverSocket;
		this->serverPoll.events = POLLIN;
		addPollFds(this->serverPoll);
		std::cout << GREEN << "Listening  on " << YELLOW <<  hostValue  << END <<  ":" << CYAN << this->serverInfo.getPorts()[i] << END << std::endl;
	}
}

/**
 * The function sends a crafted response to a socket and sets the revents field of the corresponding
 * pollfd struct to POLLERR.
 * 
 * @param i The parameter "i" is an integer that represents the index of the socket in the pollfds
 * array. It is used to access the specific socket that needs to be processed in the polloutFunc
 * function.
 * @param debug The "debug" parameter is a boolean flag that indicates whether or not to print the
 * header of the connection to the console. If it is set to true, the header will be printed. If it is
 * set to false, the header will not be printed.
 */
void	Socket::polloutFunc(int i, int debug)
{
	RequestHandler craftedRes(this->serverInfo, this->connections[this->pollfds[i].fd], this->envp, this->maxBodySizeExeeded);
	if (debug)
		std::cout << this->connections[this->pollfds[i].fd]->getHeader() << std::endl;
	send(this->pollfds[i].fd, craftedRes.getRes(), craftedRes.getResSize(), MSG_DONTWAIT);
	this->pollfds[i].revents = POLLERR;
}


/**
 * The function checks the file descriptors for incoming data or outgoing data and performs
 * corresponding actions.
 * 
 * @param debug The "debug" parameter is an integer that is used to control the level of debugging
 * information that is printed during the execution of the function. It is likely used to determine
 * whether or not to print additional information for troubleshooting purposes.
 */
void	Socket::checkFd(int debug)
{
	int res = poll(this->pollfds, this->pollPos, 1);
	if (res > 0)
	{
		for (int i = this->serverInfo.getPorts().size(); i < this->pollPos; i++)
		{
			if (this->pollfds[i].revents == POLLIN)
				this->connections[this->pollfds[i].fd]->read(this->maxBodySizeExeeded, this->serverInfo.getIntMbs());
			else if (this->pollfds[i].revents == POLLOUT)
				this->polloutFunc(i, debug);
			else
				this->pollfds[i].revents = POLLERR;
		}
		removePollFds();
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
	}
}

Socket::Socket()
{
}

Socket::~Socket()
{
	for (int i = 0; i < this->pollPos; i++)
	{
		delete this->connections[this->pollfds[i].fd];
	}
}