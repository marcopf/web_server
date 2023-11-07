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

void    RequestHandler::postRequestHandler(int matchedLocation)
{
    this->response = RequestHelper::atachStatus(METHOD_NOT_ALLOWED, "text/html", RequestHelper::fileToStr("./view/temp_err.html").c_str());
}

void    RequestHandler::deleteRequestHandler(int matchedLocation)
{
    this->response = RequestHelper::atachStatus(METHOD_NOT_ALLOWED, "text/html", RequestHelper::fileToStr("./view/temp_err.html").c_str());
}