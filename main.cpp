#include "Socket/Socket.hpp"
#include "parser/General_parser.hpp"
#include "parser/ServerConf.hpp"
#include <unistd.h>
#include <vector>
#define RED "\x1b[1;31m"
#define CYAN "\x1b[1;36m"
#define GREEN "\x1b[1;32m"
#define YELLOW "\x1b[1;33m"
#define END "\x1b[1;0m"

int main(int argc, char **argv, char **envp)
{
	General_parser parser;
	std::vector<Socket *> sockets;
	if (argc == 1)
	{
		std::cerr << RED << "Configuragtion file needed!" << std::endl << YELLOW << "to display acquired server configuration add --server-info flag" << END << std::endl;
		return (0);
	}
	if (argc == 2)
		parser = General_parser(argv[1], 0);
	if (argc == 3 && std::string(argv[2]) == "--server-info")
		parser = General_parser(argv[1], 1);
	std::vector<ServerConf> list = parser.getServersConf();
	(void)argc;
	for (unsigned int i = 0; i < list.size(); i++)
	{
		Socket *temp = new Socket(list[i], envp);
		sockets.push_back(temp);
	}
	while (42)
	{
		for (unsigned int i = 0; i < list.size(); i++)
			sockets[i]->checkFd();
	}
	for (unsigned int i = 0; i < list.size(); i++)
	{
		delete sockets[i];
	}
}