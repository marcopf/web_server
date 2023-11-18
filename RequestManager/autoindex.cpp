#include "RequestHandler.hpp"

/**
 * The function "eraseDoubleSlash" removes if present the // turing into /.
 * 
 * @param toClean The parameter "toClean" is a string that represents the input string that needs to be
 * cleaned.
 * 
 * @return a string.
 */
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

/**
 * The function `getSearchPath` returns a modified search path based on the current root and requested
 * URL.
 * 
 * @param i The parameter "i" is an integer that represents the index of the current location in the
 * "info" object.
 * 
 * @return a std::string.
 */
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

/**
 * The function "getFileInfo" takes a file path as input and returns the last modified time of the file
 * as a string.
 * 
 * @param path The parameter "path" is a C-style string that represents the path to a file.
 * 
 * @return a string that contains the last modification time of the file specified by the given path.
 * If the function is unable to get the file information, it returns an error message.
 */
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

/**
 * The function `getFileSize` takes a file path as input and returns the size of the file in bytes as a
 * string.
 * 
 * @param path The parameter "path" is a C-style string that represents the path to the file for which
 * you want to get the file size.
 * 
 * @return a string that represents the size of the file specified by the given path. If the function
 * is able to successfully retrieve the file information using the `stat` function, it will return a
 * string that contains the file size. If there is an error in retrieving the file information, it will
 * return a string that indicates the error.
 */
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

/**
 * The function "autoindex" generates an HTML page displaying the contents of a directory, including
 * file names, last modification dates, and file sizes.
 * 
 * @param directoryPath The directory path is a string that represents the path of the directory for
 * which the autoindex is being generated. It is passed as the first parameter to the `autoindex`
 * function.
 * @param i The parameter `i` in the `autoindex` function is an integer. However, it is not used in the
 * function and is passed as `(void)i`, indicating that it is not being used or referenced within the
 * function.
 * 
 * @return a string that contains HTML code for generating an autoindex page.
 */
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

/**
 * The function "callForAutoindex" checks if a given directory path is a file, and if not, it returns
 * the result of the "autoindex" function.
 * 
 * @param i The parameter "i" is an integer that represents the index of the search path. It is used to
 * retrieve the corresponding directory path from the RequestHandler object.
 * 
 * @return a status code and a message.
 */
void	RequestHandler::callForAutoindex(int i)
{
	std::string directoryPath = this->getSearchPath(i);
	if (isFile(directoryPath.c_str()))
		return (atachStatus(SUCCESS, directoryPath.c_str(), ""));
	else if (!isDir(directoryPath.c_str()))
	{
		if (this->info.getErrPage() == "null")
			return (atachStatus(NOT_FOUND, ERR_PAGE, ""));
		else
			return (atachStatus(NOT_FOUND, ERR_AUTOINDEX, ""));
	}
	return (atachStatus(SUCCESS, 0, autoindex(this->getSearchPath(i), i).c_str()));
}