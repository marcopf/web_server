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

General_parser::General_parser(const General_parser &copy)
{
	*this = copy;
}

General_parser::General_parser(){}

/**
 * The function checks if the number of opening and closing brackets in a file are equal.
 * 
 * @return a boolean value. If the number of opening brackets "{" in the "file" string is equal to the
 * number of closing brackets "}", then it returns true. Otherwise, it returns false.
 */
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

/**
 * The function "lineCheck" checks if a given line of code is valid based on certain conditions.
 * 
 * @param line The `line` parameter is a string that represents a line of code or text.
 * 
 * @return a boolean value.
 */
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

/**
 * The function checks if a given line of code contains any spaces that are not allowed in specific
 * keywords.
 * 
 * @param line The "line" parameter is a string that represents a line of code or text.
 * 
 * @return a boolean value.
 */
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

/**
 * The function `lexer_eol` checks for errors related to spaces and lines in a file.
 * 
 * @return a boolean value.
 */
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
			std::cerr << YELLOW << "Error(space): line " << counter << std::endl;
			return (false);
		}
		if (!lineCheck(data))
		{
			std::cerr << YELLOW << "Error(line): line " << counter << std::endl;
			return (false);			
		}
	}
	return (true);
}

/**
 * The General_parser constructor initializes a General_parser object by parsing a configuration file,
 * creating ServerConf objects for each server listed in the file, and optionally printing information
 * about the servers and their locations.
 * 
 * @param path The `path` parameter is a string that represents the path to a configuration file.
 * @param printInfo The "printInfo" parameter is an integer that determines whether or not to print
 * information about the server and its locations. If the value is non-zero, the information will be
 * printed. If the value is zero, the information will not be printed.
 */
General_parser::General_parser(std::string path, int printInfo):path(path)
{
	this->file = fileToStr(path);
	if (!lexer_brackets() || !lexer_eol())
	{
		std::cerr << YELLOW << "ERROR IN CONFIGURATION FILE" << END << std::endl;
		throw(3);
	}
	std::vector<std::string> server_list = getServers();
	for (unsigned int i = 0; i < server_list.size(); i++)
		this->servers.push_back(ServerConf(server_list[i]));
	if (printInfo)
	{
		for (unsigned int i = 0; i < server_list.size(); i++)
		{
			servers[i].getInfo();
			for (unsigned int j = 0; j < servers[i].locations_getter().size(); j++)
				servers[i].locations_getter()[j].getInfo();
		}
	}
}

/**
 * The function "fileToStr" reads the contents of a file and returns it as a string.
 * 
 * @param path The "path" parameter is a string that represents the file path of the file that you want
 * to convert to a string.
 * 
 * @return a string that contains the contents of the file specified by the "path" parameter.
 */
std::string		General_parser::fileToStr(std::string path)
{
	std::string			str;
	std::ifstream		f(path.c_str());
	std::ostringstream	ss;

	if (f.fail())
	{
		std::cerr << RED <<  "FILE DOES NOT EXIT!!" << END << std::endl;
		throw(2);
	}
	ss << f.rdbuf();
	str = ss.str();
	f.close();
	return (str);
}

/**
 * The function overloads the assignment operator for the General_parser class.
 * 
 * @return The object itself is being returned.
 */
General_parser &General_parser::operator=(const General_parser &copy)
{
	this->servers = copy.getServersConf();
	this->file = copy.getFile();
	this->path = copy.getPath();
	return (*this);
}

/**
 * The function `getServers` parses a file and returns a vector of strings containing server
 * configurations.
 * 
 * @return a vector of strings, which contains the server configurations found in the file.
 */
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