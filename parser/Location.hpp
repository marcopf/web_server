#ifndef LOCATION_HPP
# define LOCATION_HPP

//# include "Server.hpp"
#include <string>
#include <vector>
#include <iostream>

class Location
{
private:
	std::string root;
	std::string cgiBin;  
	std::string autoind;
	std::string method;
	std::string index;
	std::string path;
	std::string extension;
	std::string url;
	std::string file;
	std::string redirect;

public:
	Location    &operator=(const Location &copy);
	Location(const Location &copy);
	Location(std::string toParse);
	~Location();
	void						parse(std::string toParse);
	std::string					getValL(std::string file, std::string key) const;
	void						getInfo(void) const;
	std::string					getRoot(void) const;
	std::string					getCgiBin(void) const;
	std::string					getAutoind(void) const;
	std::string					getMethod(void) const;
	std::string					getIndex(void) const;
	std::string					getPath(void) const;
	std::string					getUrl(void) const;
	std::string					getRedirect(void) const;
	std::string					getExtension(void) const;
	std::string					getFile(void) const;
};


#endif