#include "Socket/Socket.hpp"
#include "parser/General_parser.hpp"
#include "parser/ServerConf.hpp"
#include <unistd.h>
#include <signal.h>
#include <vector>
#define RED "\x1b[1;31m"
#define CYAN "\x1b[1;36m"
#define GREEN "\x1b[1;32m"
#define YELLOW "\x1b[1;33m"
#define END "\x1b[1;0m"

static int keepRunning = 42;

void stop(int dummy)
{
	(void)dummy;
	keepRunning = 0;
}

/**
 * The function checks if a given path is a directory or not.
 * 
 * @param path The path parameter is a string that represents the directory path that you want to
 * check.
 * 
 * @return The function isDir returns a boolean value. It returns true if the given path is a
 * directory, and false if it is not a directory or if there was an error opening the directory.
 */
bool	isValidDir(const char *path)
{
	DIR*	temp;

	temp = opendir(path);
	if (temp == NULL)
		return (false);
	closedir(temp);
	return (true);
}

std::vector<std::string> getPossibleDir(General_parser parsedInfo)
{
	std::vector<ServerConf> servers = parsedInfo.getServersConf();
	std::vector<std::string> ret;

	for (unsigned int i = 0; i < servers.size(); i++)
	{
		if (servers[i].getPath() != "null")
		{
			ret.push_back(servers[i].getPath());
			mkdir(servers[i].getPath().c_str(), 0777);
		}
		std::vector<Location> locations = servers[i].locations_getter();
		for (unsigned int j = 0; j < locations.size(); j++)
		{
			if (locations[j].getPath() != "null")
			{
				ret.push_back(locations[j].getPath());
				mkdir(locations[j].getPath().c_str(), 0777);
			}
		}
	}
	return (ret);
}

bool	isInVect(std::vector<std::string> cont, std::string val)
{
	for (unsigned long i = 0; i < cont.size(); i++)
	{
		if (val == cont[i])
			return (true);
	}
	return (false);
}

int	checkParsedInfo(General_parser parsedInfo)
{
	std::vector<ServerConf> servers = parsedInfo.getServersConf();
	std::vector<std::string> possibleDir = getPossibleDir(parsedInfo);

	int flag = 0;

	if (servers.size() == 0)
	{
		std::cerr << RED << "NO SERVER CONFIGURATION FOUND..."<< END << std::endl;
		flag = 1;
	}
	for (unsigned int i = 0; i < servers.size(); i++)
	{
		std::vector<Location> locations = servers[i].locations_getter();
		if (servers[i].getPort() == "null")
		{
			std::cerr << RED << "PORT is missing in server[" << i << "]" << END << std::endl;
			flag = 1;
		}
		if (servers[i].getRoot() == "null" || (!isInVect(possibleDir, servers[i].getRoot()) && !isValidDir(servers[i].getRoot().c_str())))
		{
			std::cerr << RED << "ROOT is missing or invalid in server[" << i << "]" << END << std::endl;
			flag = 1;
		}
		if (servers[i].getPath() == "null")
		{
			std::cerr << RED << "PATH is missing in server[" << i << "]" << END << std::endl;
			flag = 1;
		}
		if (servers[i].getMethod() == "null")
		{
			std::cerr << RED << "METHOD is missing in server[" << i << "]" << END << std::endl;
			flag = 1;
		}	
		for (unsigned int j = 0; j < locations.size(); j++)
		{
			if (locations[j].getRoot() == "null" || (!isInVect(possibleDir, locations[j].getRoot()) && !isValidDir(locations[j].getRoot().c_str())))
			{
				std::cerr << RED << "ROOT is missing or invalid in server[" << i << "]" << "location[" << j << "]" << END << std::endl;
				flag = 1;

			}
			if (locations[j].getPath() == "null")
			{
				std::cerr << RED << "PATH is missing in server[" << i << "]" << "location[" << j << "]" << END << std::endl;
				flag = 1;
			}
			if (locations[j].getMethod() == "null")
			{
				std::cerr << RED << "METHOD is missing in server[" << i << "]" << "location[" << j << "]" << END << std::endl;
				flag = 1;
			}
		}
	}
	if (flag)
		return (1);
	return (0);
}

int main(int argc, char **argv, char **envp)
{
	General_parser	parser;
	int				debug = 0;
	std::vector<Socket *> sockets;

	signal(SIGINT, stop);
	if (argc == 1)
	{
		std::cerr << RED << "Configuragtion file needed!" << std::endl << YELLOW << "to display acquired server configuration add --debug flag" << END << std::endl;
		return (0);
	}
	if (argc == 2)
		debug = 0;
	if (argc == 3  && std::string(argv[2]) == "--debug")
		debug = 1;
	try{
		parser = General_parser(argv[1], debug);
	}catch(int e){
		return (e);
	}
	if (checkParsedInfo(parser))
	{
		std::cerr << RED << "ERROR missing important info in " << argv[1] << GREEN << std::endl <<  "path, root and method must be present in each location the main one must include also port..." << std::endl << END << std::endl;
		return (3);
	}
	std::vector<ServerConf> list = parser.getServersConf();
	(void)argc;
	for (unsigned int i = 0; i < list.size(); i++)
	{
		try{
			Socket *temp = new Socket(list[i], envp);
			sockets.push_back(temp);
		}catch(int e){
			return (e);
		}
	}
	while (keepRunning)
	{
		for (unsigned int i = 0; i < list.size(); i++)
			sockets[i]->checkFd(debug);
	}
	std::cout << std::endl << GREEN << "CLEANING UP..."  << END<< std::endl;
	for (unsigned int i = 0; i < list.size(); i++)
	{
		delete sockets[i];
	}
	std::cout << GREEN << "DONE!" << END << std::endl;

}