#ifndef General_PARSER_HPP
# define General_PARSER_HPP

#include <vector>
#include <iostream>
#include <string>
#include "Location.hpp"
#include "ServerConf.hpp"

class General_parser
{
private:
	std::vector<ServerConf> servers;
	std::string path;
	std::string file;
public:
	General_parser(std::string path);
	General_parser(const General_parser &copy);
	General_parser &operator=(const General_parser &copy);
	~General_parser();
	bool						lexer_brackets();
	bool						lexer_eol();
	std::string					fileToStr(std::string path);
	std::vector<std::string>	getServers(void);
	std::vector<ServerConf>		servers_getter(void) const;
	std::string					getPath(void) const;
	std::string					getFile(void) const;
	std::vector<ServerConf>		getServersConf(void) const;
};

#endif