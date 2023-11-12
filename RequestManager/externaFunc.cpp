#include "RequestHandler.hpp"

bool fileExists(const char* fileName) {
    std::ifstream file(fileName);
    return file.good();
}

std::string	getCurrentRoot(ServerConf serv, int i)
{
	if (i > -1 && serv.locations_getter()[i].getRoot() != "null")
		return (serv.locations_getter()[i].getRoot());
	return (serv.getRoot());
}

bool	isDir(const char *path)
{
	DIR*	temp;

	temp = opendir(path);
	if (temp == NULL)
		return (false);
	closedir(temp);
	return (true);
}

bool	isFile(const char *path)
{
	if (!isDir(path) && fileExists(path))
		return (true);
	return (false);	
}