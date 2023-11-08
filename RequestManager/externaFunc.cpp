#include "RequestHandler.hpp"

bool fileExists(const char* fileName) {
    std::ifstream file(fileName);
    return file.good();
}

std::string atachStatus(const char *status, const char *type, const char *body)
{
	std::stringstream ss;
	std::string status_s = status, body_s = body;
	ss << "\r\nContent-length: " << body_s.length() << "\r\n\r\n";
	std::string res = status_s + ss.str() + body_s + "\r\n";
	return (res);
}

std::string	getCurrentRoot(ServerConf serv, int i)
{
	if (i > -1 && serv.locations_getter()[i].getRoot() != "null")
		return (serv.locations_getter()[i].getRoot());
	return (serv.getRoot());
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