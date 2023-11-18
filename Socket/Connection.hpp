#ifndef CONNECTION_HPP
# define CONNECTION_HPP

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

class Connection
{
private:
	unsigned long	newBufferLen;
	unsigned long	oldBufferLen;
	unsigned long	headerSize;
	unsigned long	bodySize;
	std::string  	header;
	int				fd;
	char			*buffer;
	char			*body;
	struct pollfd	*pollfd;
public:
	unsigned long	getBodySize(void);
	std::string		getHeader(void);
	char			*getBody(void);
	void			read(int &maxBodySizeExeeded, int maxBodySize);
	void			addToBuffer(const char *toAdd);
	int				handleBody(int &maxBodySizeExeeded, int maxBodySize);
	Connection(const Connection &cpy);
	Connection &operator=(const Connection &cpy);
	Connection(struct pollfd *pollfd);
	~Connection();
};

#endif