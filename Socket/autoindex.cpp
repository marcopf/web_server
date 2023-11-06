#include "RequestHandler.hpp"

std::string	eraseDoubleSlash(std::string toClean)
{
    std::string toRet;

	if (toClean.find("//") != std::string::npos)
    {
        toRet = toClean.substr(0, toClean.find("//"));
        toRet = toClean.substr(toClean.find("//") + 2);
		return (toRet);
    }
	return (toClean);
}

std::string	RequestHandler::getSearchPath(int i)
{
	unsigned long			pos;
	std::string				ret;
	std::string::iterator	it;

	ret = getCurrentRoot(this->info, i);
	if (ret[ret.length() - 1] == '/')
	{
		pos = ret.find_last_of("/");
		it = ret.begin() + pos;
		if (pos != std::string::npos)
			ret.erase(it, ret.end());
	}
	if (i >= 0 && this->info.locations_getter()[i].getUrl() != "null")
		ret += this->requestedUrl.substr(this->info.locations_getter()[i].getUrl().length());
	else
	{
		if (ret[0] == '.' && strncmp(ret.substr(1).c_str(), this->requestedUrl.c_str(), ret.substr(1).length()) != 0)
			ret += this->requestedUrl;
		else if (ret[0] != '.' && strncmp(ret.c_str(), this->requestedUrl.c_str(), ret.length()))
			ret += this->requestedUrl;
		else if (ret[0] != '.')
			ret = this->requestedUrl;
		else
			ret = "." + this->requestedUrl;
	}
	return (ret);
}

std::string	getFileInfo(const char *path)
{
	struct stat			fileInfo;
	std::stringstream	ss;

	if (stat(path, &fileInfo) == 0)
		ss << std::ctime(&fileInfo.st_mtime);
	else
		ss << "Error: unable to get info...";
	return (ss.str());
}

std::string	getFileSize(const char *path)
{
	struct stat			fileInfo;
	std::stringstream	ss;

	if (stat(path, &fileInfo) == 0)
		ss << "		" << fileInfo.st_size;
	else
		ss << "Error: unable to get info...";
	return (ss.str());
}

std::string	RequestHandler::autoindex(std::string directoryPath, int i)
{
	std::stringstream ss;

	(void)i;
    DIR* dir = opendir(directoryPath.c_str());
    if (dir == NULL)
	{
        std::cerr << "Failed to open directory." << std::endl;
        return "";
    }
    struct dirent* entry;
	ss << "<h1 style=\"font-family:arial;\"> Index Of " << directoryPath << "</h1>\n";
	ss << "<hr>\n";
	ss << "<div style=\"display:flex; flex-direction: column; font-family:arial;\">";
	ss << "<div style=\"display:flex; font-weight:900; color: black; justify-content: start\"><span style=\"width:30vw; font-size:15px;\">" << "FILE NAME" << "</span><span style=\"width: 40vw;\">"<< "LAST MODIFICATION" <<"</span><span style=\"width: 30vw;\">"<< "FILE SIZE" <<"</span></div>\n";
    while ((entry = readdir(dir)) != NULL)
	{
		if (directoryPath[directoryPath.length() - 1] != '/')
			directoryPath += "/";
		if (isDir((directoryPath + entry->d_name).c_str()))
		{
			if (directoryPath[0] == '.')
        		ss << "<div style=\"display:flex; justify-content: start\"><a style=\"width:30vw; font-size:15px; color: crimson;\" href=\"/" << (eraseDoubleSlash(this->requestedUrl + "/" + entry->d_name)).substr(1) << "\">" << entry->d_name << "/</a><span style=\"width: 40vw;\">"<< getFileInfo((directoryPath + entry->d_name).c_str()) <<"</span><span style=\"width: 30vw;\">"<< getFileSize((directoryPath + entry->d_name).c_str()) <<"</span></div>\n";
			else
				ss << "<div style=\"display:flex; justify-content: start\"><a style=\"width:30vw; font-size:15px; color: crimson;\" href=\"" << (eraseDoubleSlash(this->requestedUrl + "/" + entry->d_name)) << "\">" <<  entry->d_name << "/</a><span style=\"width: 40vw;\">"<< getFileInfo((directoryPath + entry->d_name).c_str()) <<"</span><span style=\"width: 30vw;\">"<< getFileSize((directoryPath + entry->d_name).c_str()) <<"</span></div>\n";
		}
		else
		{
			if (directoryPath[0] == '.')
        		ss << "<div style=\"display:flex; justify-content: start\"><a style=\"width:30vw; font-size:15px; color: black;\" href=\"/" << (eraseDoubleSlash(this->requestedUrl + "/" + entry->d_name)).substr(1) << "\">" <<  entry->d_name << "</a><span style=\"width: 40vw;\">"<< getFileInfo((directoryPath + entry->d_name).c_str()) <<"</span><span style=\"width: 30vw;\">"<< getFileSize((directoryPath + entry->d_name).c_str()) <<"</span></div>\n";
			else
				ss << "<div style=\"display:flex; justify-content: start\"><a style=\"width:30vw; font-size:15px; color: black;\" href=\"" << (eraseDoubleSlash(this->requestedUrl + "/" + entry->d_name)) << "\">" <<  entry->d_name << "</a><span style=\"width: 40vw;\">"<< getFileInfo((directoryPath + entry->d_name).c_str()) <<"</span><span style=\"width: 30vw;\">"<< getFileSize((directoryPath + entry->d_name).c_str()) <<"</span></div>\n";
		}
    }
	ss << "</div>\n";
	ss << "<hr>\n";
    closedir(dir);
	return (ss.str());
}

std::string	RequestHandler::callForAutoindex(int i)
{
	std::string directoryPath = this->getSearchPath(i);

	if (isFile(directoryPath.c_str()))
		return (atachStatus(SUCCESS, PLAIN, fileToStr(directoryPath.c_str()).c_str()));
	else if (!isDir(directoryPath.c_str()))
	{
		if (this->info.getErrPage() == "null")
			return (atachStatus(NOT_FOUND, HTML, fileToStr(ERR_PAGE).c_str()));
		else
			return (atachStatus(NOT_FOUND, HTML, fileToStr(ERR_AUTOINDEX).c_str()));
	}
	return (atachStatus(NOT_FOUND, HTML, autoindex(this->getSearchPath(i), i).c_str()));
}