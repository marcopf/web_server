#include "RequestHandler.hpp"

/**
 * The getRequestHandler function handles different types of GET requests and returns the appropriate
 * response based on the matched location and requested URL.
 * 
 * @param matchedLocation The parameter `matchedLocation` is an integer that represents the index of
 * the matched location in the `locations` vector. It is used to retrieve information about the
 * location from the `info` object. If `matchedLocation` is -1, it means that no location was matched.
 */
void    RequestHandler::getRequestHandler(int matchedLocation)
{
    unsigned long   pos = this->requestedUrl.find("cgi-bin");
    std::string     baseCgiPath = matchedLocation > -1 ? this->info.locations_getter()[matchedLocation].getCgiBin() : "null";
    std::string     fileName;
    std::string		completePath = getSearchPath(matchedLocation);

    addNewEnvp();
    if (matchedLocation == -1 && this->requestedUrl == "/")
	{
		if (this->info.getIndex() != "null" && fileExists(this->info.getIndex().c_str()))
			atachStatus(SUCCESS, this->info.getIndex().c_str(), "");
		else
			atachStatus(SUCCESS, WELCOME, "");
	}
    else if (pos != std::string::npos && baseCgiPath != "null")
    {
        fileName = this->requestedUrl.substr(pos + 7);
        executeFile(baseCgiPath + fileName, this->envp, 0);
    }
    else if (!isDir(completePath.c_str()) && fileExists(completePath.c_str()))
		atachStatus(SUCCESS, completePath.c_str(), "");
	else if (isDir(completePath.c_str()))
	{
		if (matchedLocation >= 0 && this->info.locations_getter()[matchedLocation].getIndex() != "null" && fileExists(this->info.locations_getter()[matchedLocation].getIndex().c_str()))
			atachStatus(SUCCESS, this->info.locations_getter()[matchedLocation].getIndex().c_str(), "");
		else
			atachStatus(SUCCESS, WELCOME, "");
	}
	else if (this->info.getErrPage() != "null" && fileExists(this->info.getErrPage().c_str()))
		atachStatus(NOT_FOUND, this->info.getErrPage().c_str(), "");
	else
		atachStatus(NOT_FOUND, ERR_PAGE, "");
}

/**
 * The function `getBodyInfo` returns the index of the character after the third occurrence of a
 * newline character in a given character array.
 * 
 * @param body The `body` parameter is a pointer to a character array that represents a body of text.
 * It is assumed that the text is stored as a null-terminated string.
 * @param len The parameter `len` represents the length of the `body` string.
 * 
 * @return the index of the character in the body string that is two positions after the third
 * occurrence of the newline character ('\n').
 */
int	getBodyInfo(char *body, int len)
{
	int i;
	int	counter = 0;

	i = 0;
	while (i < len)
	{
		if (body[i] == '\n')
			counter++;
		if (counter == 3)
			break ;
		i++;
	}
	return (i + 2);
}

/**
 * The function "findFileName" extracts the file name from a given string that contains information
 * about a file.
 * 
 * @param bodyInfo The parameter `bodyInfo` is a string that contains information about a file,
 * including the file name.
 * 
 * @return a string, which is the file name extracted from the bodyInfo string.
 */
std::string	findFileName(std::string bodyInfo)
{
	std::string	fileName;
	unsigned long start = bodyInfo.find("filename=\"") + 10;
	unsigned long end;

	if (start != std::string::npos)
	{
		end = bodyInfo.find("\"", start);
		fileName = bodyInfo.substr(start, end - start);
	}
	return (fileName);
}

/**
 * The function saves the body of a generic request to a file if the request does not contain a
 * boundary.
 * 
 * @param toAdd The "toAdd" parameter is a string that represents the file path where the file will be
 * saved.
 * 
 * @return a boolean value.
 */
bool	RequestHandler::saveGenericBody(std::string toAdd)
{
	static int			fileCount;
	std::stringstream	filename;

	if (this->req->getHeader().find("boundary") == std::string::npos)
	{
		filename << "postFile(" <<  fileCount++ << ")";
		toAdd = toAdd + "/" + filename.str();
		std::ofstream	newFile(toAdd.c_str(), std::ios::binary);
		newFile.write(this->req->getBody(), this->req->getBodySize());
		newFile.close();
		atachStatus(SUCCESS, WELCOME, "");
		return true;
	}
	return false;
}

/**
 * The function `saveMultiPartBody` saves a multi-part body of an HTTP request to a file.
 * 
 * @param bodyInfo A string containing information about the multipart body, including the boundary and
 * other details.
 * @param path The `path` parameter is a string that represents the directory path where the file will
 * be saved.
 * @param toAdd The "toAdd" parameter is a string that represents the directory where the file should
 * be saved. If it is an empty string or "null", the file will be saved in the current directory.
 * Otherwise, the file will be saved in the specified directory.
 */
void	RequestHandler::saveMultiPartBody(std::string bodyInfo, std::string path, std::string toAdd)
{
	int				bodyOffeset = getBodyInfo(this->req->getBody(), this->req->getBodySize());
	char			*cpy = this->req->getBody() + bodyOffeset;
	std::string		boundary = bodyInfo.substr(0, bodyInfo.find("\r\n"));
	unsigned long	fileSize = headerLen(this->req->getBody() + bodyOffeset, (boundary + "--").c_str(), this->req->getBodySize() - bodyOffeset) - 2;
	std::string		fileName = findFileName(bodyInfo);

	addNewEnvp();
	if (toAdd == "" || toAdd == "null")
		path = fileName.c_str();
	path = toAdd + "/" + fileName.c_str();
	std::ofstream	newFile(path.c_str(), std::ios::binary);
	if (cpy[0] == 10 || cpy[0] == 13)
		newFile.write(cpy + 1, fileSize);
	else
		newFile.write(cpy, fileSize - 1);
	newFile.close();
	atachStatus(SUCCESS, WELCOME, "");
}

/**
 * The postRequestHandler function handles POST requests by saving the request body and executing CGI
 * scripts if necessary.
 * 
 * @param matchedLocation The parameter `matchedLocation` is an integer that represents the index of
 * the matched location in the `locations` vector. It is used to determine the appropriate CGI bin path
 * for executing CGI scripts. If `matchedLocation` is -1, it means that no location was matched and the
 * default CGI bin
 * 
 * @return nothing (void).
 */
void    RequestHandler::postRequestHandler(int matchedLocation)
{
	if (!this->req->getBody())
	{
		atachStatus(SUCCESS, WELCOME, "");
		return ;
	}
	int				bodyOffeset = getBodyInfo(this->req->getBody(), this->req->getBodySize());
	std::string 	bodyInfo = std::string(this->req->getBody(), bodyOffeset);
	std::string		toAdd, path;
	std::string     baseCgiPath = matchedLocation > -1 ? this->info.locations_getter()[matchedLocation].getCgiBin() : "null";
	unsigned long	pos = this->requestedUrl.find("cgi-bin");
	if (matchedLocation == -1)
		toAdd = this->info.getPath();
	else
		toAdd = this->info.locations_getter()[matchedLocation].getPath();
	if (toAdd != "" && toAdd != "null")
		mkdir(toAdd.c_str(), 0777);
	if (!this->saveGenericBody(toAdd))
		this->saveMultiPartBody(bodyInfo, path, toAdd);
    if (pos != std::string::npos && baseCgiPath != "null")
    {
		std::string cgiFile = this->requestedUrl.substr(pos + 7);
        executeFile(baseCgiPath + cgiFile, this->envp, this->req->getBody());
		return ;
    }
}

/**
 * The deleteRequestHandler function handles the deletion of a file based on the matched location.
 * 
 * @param matchedLocation The parameter `matchedLocation` is an integer that represents the index of a
 * matched location. It is used to determine the upload directory for deleting a file. If
 * `matchedLocation` is -1, it means that no location was matched and the upload directory will be set
 * to the default path. Otherwise
 * 
 * @return nothing (void).
 */
void    RequestHandler::deleteRequestHandler(int matchedLocation)
{
	// (void)matchedLocation;
	std::string	uploadDirectory = matchedLocation == -1 ? this->info.getPath() : this->info.locations_getter()[matchedLocation].getPath();

	addNewEnvp();
	for (unsigned long i = 0; i < this->envp.size(); i++)
	{
		if ("fileToDelete" == this->envp[i].substr(0, 12))
		{
			if (this->envp[i].substr(13).find("..") == std::string::npos &&  unlink((uploadDirectory + "/" + this->envp[i].substr(13)).c_str()) == -1)
				atachStatus(NOT_FOUND_DELETE, "./view/displayError/err.html", "");
			else if (this->envp[i].substr(13).find("..") == std::string::npos)
				atachStatus(SUCCESS, WELCOME, "");
			else
				atachStatus(NOT_FOUND, "./view/displayError/err.html", "");
			return ;
		}
	}
    atachStatus(NOT_FOUND, "./view/displayError/err.html", "");
}