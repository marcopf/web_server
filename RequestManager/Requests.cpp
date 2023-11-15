#include "RequestHandler.hpp"

void    RequestHandler::getRequestHandler(int matchedLocation)
{
    unsigned long   pos = this->requestedUrl.find("cgi-bin");
    std::string     baseCgiPath = matchedLocation > -1 ? this->info.locations_getter()[matchedLocation].getCgiBin() : "null";
    std::string     fileName;
    std::string		completePath = getSearchPath(matchedLocation);

    addNewEnvp();
	std::cout << "search: " << completePath << std::endl;
    if (matchedLocation == -1 && this->requestedUrl == "/")
	{
		if (this->info.getIndex() != "null" && fileExists(this->info.getIndex().c_str()))
			this->response = atachStatus(SUCCESS, fileToStr(this->info.getIndex().c_str()).c_str());
		else
			this->response = atachStatus(SUCCESS, fileToStr(WELCOME).c_str());
	}
    else if (pos != std::string::npos && baseCgiPath != "null")
    {
        fileName = this->requestedUrl.substr(pos + 7);
        this->response = executeFile(baseCgiPath + fileName, this->envp, 0);
    }
    else if (!isDir(completePath.c_str()) && fileExists(completePath.c_str()))
		this->response = atachStatus(SUCCESS,fileToStr(completePath.c_str()).c_str());
	else if (isDir(completePath.c_str()))
	{
		if (matchedLocation >= 0 && this->info.locations_getter()[matchedLocation].getIndex() != "null" && fileExists(this->info.locations_getter()[matchedLocation].getIndex().c_str()))
			this->response = atachStatus(SUCCESS, fileToStr(this->info.locations_getter()[matchedLocation].getIndex().c_str()).c_str());
		else
			this->response = atachStatus(SUCCESS, fileToStr(WELCOME).c_str());
	}
	else if (this->info.getErrPage() != "null" && fileExists(this->info.getErrPage().c_str()))
		this->response = atachStatus(NOT_FOUND,fileToStr(this->info.getErrPage().c_str()).c_str());
	else
		this->response = atachStatus(NOT_FOUND, fileToStr(ERR_PAGE).c_str());
}

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

void    RequestHandler::postRequestHandler(int matchedLocation)
{
	if (!this->req->getBody())
	{
		this->response =  atachStatus(SUCCESS, fileToStr("./view/welcome.html").c_str());
		return ;
	}
	int				bodyOffeset = getBodyInfo(this->req->getBody(), this->req->getBodySize());
	std::string 	bodyInfo = std::string(this->req->getBody(), bodyOffeset);
	std::string		boundary = bodyInfo.substr(0, bodyInfo.find("\r\n"));
	std::string		fileName = findFileName(bodyInfo);
	unsigned long	fileSize = headerLen(this->req->getBody() + bodyOffeset, (boundary + "--").c_str(), this->req->getBodySize() - bodyOffeset) - 2;
	char			*cpy = this->req->getBody() + bodyOffeset;
	std::string     baseCgiPath = matchedLocation > -1 ? this->info.locations_getter()[matchedLocation].getCgiBin() : "null";
	unsigned long	pos = this->requestedUrl.find("cgi-bin");
	std::string		toAdd, path;

	addNewEnvp();
    if (pos != std::string::npos && baseCgiPath != "null")
    {
        this->response = executeFile(baseCgiPath + fileName, this->envp, this->req->getBody());
		return ;
    }

	if (matchedLocation == -1)
		toAdd = this->info.getPath();
	else
		toAdd = this->info.locations_getter()[matchedLocation].getPath();
	if (toAdd != "" && toAdd != "null")
	{
		path = toAdd + "/" + fileName.c_str();
		mkdir(toAdd.c_str(), 0777);
	}
	else
		path = fileName.c_str();
	std::ofstream	newFile(path.c_str(), std::ios::binary);
	if (cpy[0] == 10 || cpy[0] == 13)
		newFile.write(cpy + 1, fileSize - 1);
	else
		newFile.write(cpy, fileSize);
	newFile.close();
	this->response =  atachStatus(SUCCESS, fileToStr("./view/welcome.html").c_str());
}

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
				this->response = atachStatus(NOT_FOUND_DELETE, fileToStr("./view/displayError/err.html").c_str());
			else if (this->envp[i].substr(13).find("..") == std::string::npos)
				this->response = atachStatus(SUCCESS, fileToStr("./view/welcome.html").c_str());
			else
				this->response = atachStatus(NOT_FOUND, fileToStr("./view/displayError/err.html").c_str());
			return ;
		}
	}
    this->response = atachStatus(NOT_FOUND, fileToStr("./view/displayError/err.html").c_str());
}