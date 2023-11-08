#include "../Socket/RequestHelper.hpp"
#include "RequestHandler.hpp"

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
			this->response = atachStatus(SUCCESS, HTML, fileToStr(this->info.getIndex().c_str()).c_str());
		else
			this->response = atachStatus(SUCCESS, HTML, fileToStr(WELCOME).c_str());
	}
    else if (pos != std::string::npos && baseCgiPath != "null")
    {
        fileName = this->requestedUrl.substr(pos + 7);
        this->response = RequestHelper::executeFile(baseCgiPath + fileName, this->envp);
    }
    else if (!isDir(completePath.c_str()) && fileExists(completePath.c_str()))
		this->response = atachStatus(SUCCESS, HTML,fileToStr(completePath.c_str()).c_str());
	else if (isDir(completePath.c_str()))
	{
		if (matchedLocation >= 0 && this->info.locations_getter()[matchedLocation].getIndex() != "null" && fileExists(this->info.locations_getter()[matchedLocation].getIndex().c_str()))
			this->response = atachStatus(SUCCESS, HTML, fileToStr(this->info.locations_getter()[matchedLocation].getIndex().c_str()).c_str());
		else
			this->response = atachStatus(SUCCESS, HTML, fileToStr(WELCOME).c_str());
	}
	else if (this->info.getErrPage() != "null" && fileExists(this->info.getErrPage().c_str()))
		this->response = atachStatus(NOT_FOUND, HTML,fileToStr(this->info.getErrPage().c_str()).c_str());
	else
		this->response = atachStatus(NOT_FOUND, HTML, fileToStr(ERR_PAGE).c_str());
}

void	RequestHandler::createFile(std::stringstream &filename, int matchedLocation, int &isFile, int &counter, std::string &ret)
{
	std::string toAdd, path;

	if (matchedLocation == -1)
		toAdd = this->info.getPath();
	else
		toAdd = this->info.locations_getter()[matchedLocation].getPath();
	if (toAdd != "" && toAdd != "null")
	{
		path = toAdd + "/" + filename.str().c_str();
		mkdir(toAdd.c_str(), 0777);
	}
	else
		path = filename.str().c_str();
	std::ofstream newFile(path.c_str());
	newFile << ret;
	newFile.close();
	counter = 1;
	ret = "";
	filename.str("");
	isFile = 2;
}

void    RequestHandler::postRequestHandler(int matchedLocation)
{
	static int			newFileName;
	int					isFile = 2;
	int					counter = 1;
	std::stringstream	body(this->request.substr(this->request.find("\r\n\r\n") + 4));
	std::stringstream	ss;
	std::string 		ret;
	std::string 		line;
	std::string 		boundary;

	getline(body, boundary, '\r');
	while (getline(body, line, '\r'))
	{
		if (line.find(boundary) != std::string::npos)
			createFile(ss, matchedLocation, isFile, counter, ret);
		if (line.find("Content-Disposition: form-data;") != std::string::npos)
		{
			if (line.find("filename=\"") == std::string::npos)
				ss << "postFile(" << newFileName << ")";
			unsigned long start = line.find("filename=\"") + 10;
			if (start - 10 != std::string::npos)
			{
				isFile = 3;
				unsigned long end = line.find("\"", start + 1);
				ss << line.substr(start, end - start);
			}
			newFileName++;
		}
		if (counter > isFile)	
			ret += line == "\n" ? "" : line;
		counter++;
	}
	this->response =  atachStatus(SUCCESS, PLAIN, ret.c_str());
	return ;
}

void    RequestHandler::deleteRequestHandler(int matchedLocation)
{
    this->response = RequestHelper::atachStatus(METHOD_NOT_ALLOWED, "text/html", RequestHelper::fileToStr("./view/temp_err.html").c_str());
}