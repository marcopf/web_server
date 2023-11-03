#include "General_parser.hpp"
#include "Location.hpp"
#include "ServerConf.hpp"
#include <fstream>
#include <vector>

std::vector<ServerConf>		 General_parser::getServersConf(void) const
{
	return (this->servers);
}
std::string					General_parser::getPath(void) const
{
	return (this->path);
}
std::string					General_parser::getFile(void) const
{
	return (this->file);
}

bool	General_parser::lexer_brackets(void)
{
	int openings, closing, i = 0, j = 0;

	openings = this->file.find("{");
	while ((unsigned long)openings != std::string::npos)
	{
		i++;
		openings = this->file.find("{", openings + 1);
	}
	closing = this->file.find("}");
	while ((unsigned long)closing != std::string::npos)
	{
		j++;
		closing = this->file.find("}", closing + 1);
	}
	if (i == j)
		return (true);
	return (false);
}

bool	lineCheck(std::string line)
{
	std::string ending;

	if (line.empty())
		return (true);
	ending = line.substr(line.length() - 1);
	if (line.find("location") != std::string::npos && line.find("{") == std::string::npos)
		return (false);
	if (line.find("server_conf") != std::string::npos && line.find("{") == std::string::npos)
		return (false);
	if (ending == "}" || ending == ";" || ending == "{")
		return (true);
	return (false);
}

bool	space_check(std::string line)
{
	std::string key_list[] = {"location", "listen", "host", "port", "server_name", "err_page", "max_body_size", "root", "autoind", "index", "path", "url", "method", "extension"};
	std::string sub;

	if (line.empty())
		return (true);
	if (line.find(" ") == std::string::npos && line.find("server_conf") == std::string::npos && line.find("location") == std::string::npos && line.find("}") == std::string::npos)
		return (false);
	sub = line.substr(0, line.find(" ") + 1);
	for (int i = 0; i < 14; i++)
	{
		if (sub.find(key_list[i]) != std::string::npos && sub[sub.find(key_list[i]) + key_list[i].length()] != ' ')
			return (false);
	}
	return (true);
}

bool	General_parser::lexer_eol(void)
{
	std::fstream	ss;
	std::string		data;
	int				counter = 0;

	ss.open(this->path.c_str(), std::ios::in);
	while (getline(ss, data, '\n'))
	{
		counter++;
		if (!space_check(data))
		{
			std::cerr << "Error(space): line " << counter << std::endl;
			return (false);
		}
		if (!lineCheck(data))
		{
			std::cerr << "Error(line): line " << counter << std::endl;
			return (false);			
		}
	}
	return (true);
}

General_parser::General_parser(std::string path):path(path)
{
	this->file = fileToStr(path);
	if (!lexer_brackets() || !lexer_eol())
	{
		std::cerr << "Error: incorrect configuration file" << std::endl;
		return ;
	}
	std::vector<std::string> server_list = getServers();
	for (unsigned int i = 0; i < server_list.size(); i++)
		this->servers.push_back(ServerConf(server_list[i]));
	for (unsigned int i = 0; i < server_list.size(); i++)
	{
		servers[i].getInfo();
		for (unsigned int j = 0; j < servers[i].locations_getter().size(); j++)
			servers[i].locations_getter()[j].getInfo();
	}
}

General_parser::General_parser(const General_parser &copy)
{
	*this = copy;
}

std::string		General_parser::fileToStr(std::string path)
{
	std::string			str;
	std::ifstream		f(path.c_str());
	std::ostringstream	ss;

	ss << f.rdbuf();
	str = ss.str();
	f.close();
	return (str);
}

General_parser &General_parser::operator=(const General_parser &copy)
{
	this->servers = copy.getServersConf();
	this->file = copy.getFile();
	this->path = copy.getPath();
	return (*this);
}

std::vector<std::string>	General_parser::getServers(void)
{
	int openings, openings2;
	std::string	target = "server_conf";
	std::string toAdd;
	std::vector<std::string> server_list;

	openings = this->file.find(target);
	while ((unsigned long)openings != std::string::npos)
	{
		openings2 = this->file.find(target, openings + target.length());
		if ((unsigned long)openings2 == std::string::npos)
		{
			toAdd = this->file.substr(openings);
			server_list.push_back(toAdd);
			break;
		}
		toAdd = this->file.substr(openings, openings2 - openings);
		server_list.push_back(toAdd);
		openings = openings2;
	}
	return (server_list);
}

General_parser::~General_parser()
{

}