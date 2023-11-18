#ifndef PARSER_HPP
# define PARSER_HPP

#include <string>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <vector>
#include "Location.hpp"
#include <limits.h>

#define RED "\x1b[1;31m"
#define CYAN "\x1b[1;36m"
#define GREEN "\x1b[1;32m"
#define YELLOW "\x1b[1;33m"
#define END "\x1b[1;0m"

class ServerConf
{
private:
	std::string listen;
	std::string port;
	std::string	path;
	std::string host;
	std::string	method;
	std::string server_name;
	std::string err_page;
	std::string max_body_size;
	std::string root;
	std::string	autoind;
	std::string index;
	std::string file;
	std::vector<Location> locations;

public:
	ServerConf(std::string file);
	ServerConf(void);
	ServerConf(const ServerConf &cpy);
	std::vector<std::string>	getLocations(void);
	std::string					getFile(void) const;
	void						getInfo(void);
	std::vector<Location>		locations_getter(void) const;
	std::string					getValP(std::string file, std::string key);
	void						parse(void);
	int							getIntPort(void) const;
	int							getIntMbs(void) const;
	const ServerConf& 			operator=(const ServerConf &cpy);
	std::vector<int>			getPorts() const;
	std::string					getPort() const;
	std::string					getRoot() const;
	std::string					getPath() const;
	std::string					getMethod() const;
	std::string					getListen() const;
	std::string					getHost() const;
	std::string					getServerName() const;
	std::string					getErrPage() const;
	std::string					getMaxBodySize() const;
	std::string					getIndex() const;
	std::string					getAutoind() const;
	~ServerConf();
};

#endif