#include "Location.hpp"
#include <sstream>
#include <stdlib.h>

std::string		Location::getRoot(void) const
{
	return(this->root);
}
std::string                 Location::getAutoind(void) const
{
	return(this->autoind);
}
std::string                 Location::getCgiBin(void) const
{
	return(this->cgiBin);
}
std::string                 Location::getMethod(void) const
{
	return(this->method);
}
std::string                 Location::getIndex(void) const
{
	return(this->index);
}
std::string                 Location::getPath(void) const
{
	return(this->path);
}
std::string                 Location::getUrl(void) const
{
	return(this->url);
}
std::string                 Location::getFile(void) const
{
	return(this->file);
}
std::string                 Location::getExtension(void) const
{
	return(this->extension);
}
std::string                 Location::getRedirect(void) const
{
	return(this->redirect);
}

Location::Location(std::string toParse):file(toParse)
{
    this->root = "";
    this->autoind = "";
    this->method = "";
    this->index = "";
    this->path = "";
    this->extension = "";
	this->file = "";
	this->url = "";
	this->redirect = "";
	this->parse(toParse);
}

void Location::getInfo(void) const
{
	std::cout << "----LOCATION----" << std::endl; 
	std::cout << "root: " << this->root << "\nurl: " << this->url << "\nautoind: " << this->autoind << "\nmethod: " << this->method << "\nindex: " << this->index << "\npath: " << this->path << "\nextension: " << this->extension << "\nredirect: " << this->redirect << "\ncgi-bin: " << this->cgiBin << std::endl;
	std::cout << "----------------\n" << std::endl; 
}

Location &Location::operator=(const Location &copy)
{
    this->root = copy.getRoot();
    this->autoind = copy.getAutoind();
    this->method = copy.getMethod();
    this->index = copy.getIndex();
    this->path = copy.getPath();
	this->file = copy.getFile();
	this->url = copy.getUrl();
    this->extension = copy.getExtension();
	this->redirect = copy.getRedirect();
	this->cgiBin = copy.getCgiBin();
    return(*this);
}

/**
 * The function `getLineKey` takes a line of text as input and returns the key (or keyword) found in
 * that line, based on a predefined list of possible keys.
 * 
 * @param line The `line` parameter is a string that represents a line of text.
 * 
 * @return The function `getLineKey` returns a string. If the substring `sub` contains any of the
 * keywords in the `key_list` array, then the corresponding keyword is returned. If none of the
 * keywords are found in the substring, then the string "null" is returned.
 */
std::string	getLineKey(std::string line)
{
	std::string sub = line.substr(0, line.find(" "));
	std::string key_list [] = {"location", "listen", "host", "port", "server_name", "err_page", "max_body_size", "root", "autoind", "index", "path", "url", "method", "extension", "redirect", "cgi-bin"};

	for (int i = 0; i < 16; i++)
	{
		if (sub.find(key_list[i]) != std::string::npos)
			return (key_list[i]);
	}
	return ("null");
}

/**
 * The function `getValL` takes a file and a key as input and returns the corresponding value
 * associated with that key in the file.
 * 
 * @param file The `file` parameter is a string that represents the file from which the data is being
 * read.
 * @param key The "key" parameter is a string that represents the key you are searching for in the
 * "file" string.
 * 
 * @return a string value.
 */
std::string	Location::getValL(std::string file, std::string key) const
{
	std::stringstream	ss(file);
	std::string			sub;
	std::string			data;

	while (getline(ss, data, '\n'))
	{
		if (key != getLineKey(data))
			continue ;
		if (data.find(key) != std::string::npos && data.find(" ") != std::string::npos)
		{
			sub = data.substr((data.find(key) + key.length()) + 1, data.length() - (data.find(key) + key.length()) - 2);
			for (unsigned int i = 0; i < sub.length(); i++)	
			{
				if (!isspace(sub[i]))
					return (data.substr(data.find(" ") + 1, data.length() - (data.find(key) + key.length()) - 2));
			}
			return ("null");
		}
		else if (data.find(key) != std::string::npos && data.find(";") != std::string::npos)
			return (data.substr((data.find(key) + key.length()), data.find(";") - 1));
	}
	return ("null");
}

void    Location::parse(std::string file)
{
	this->method = getValL(file, "method");
	this->root = getValL(file, "root");
	this->index = getValL(file, "index");
	this->autoind = getValL(file, "autoind");
	this->path = getValL(file, "path");
	this->extension = getValL(file, "extension");
	this->url = getValL(file, "location");
	this->redirect = getValL(file, "redirect");
	this->cgiBin = getValL(file, "cgi-bin");
}

Location::Location(const Location &copy)
{
    *this = copy;
}

Location::~Location()
{
}
