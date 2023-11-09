

#include "Socket/Socket.hpp"
#include "parser/General_parser.hpp"
#include "parser/ServerConf.hpp"
#include <unistd.h>
#include <vector>

int main(int argc, char **argv, char **envp)
{
	General_parser p1(argv[1]);
	std::vector<Socket *> sockets;
	std::vector<ServerConf> list = p1.getServersConf();
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