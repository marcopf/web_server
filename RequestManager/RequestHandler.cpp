#include "RequestHandler.hpp"

ServerConf RequestHandler::getInfo() const
{
    return (this->info);
}

std::string RequestHandler::getReq() const
{
    return (this->request);
}
std::string RequestHandler::getRes() const
{
    return (this->response);
}
std::vector<std::string> RequestHandler::getEnvp() const
{
    return (this->envp);
}
std::string RequestHandler::getRequestedUrl() const
{
    return (this->requestedUrl);
}
std::string RequestHandler::getMethod() const
{
    return (this->method);
}

void RequestHandler::setMethod(std::string method)
{
    this->method = method;
}
void RequestHandler::setRequestedUrl(std::string url)
{
    this->requestedUrl = url;
}

RequestHandler &RequestHandler::operator=(const RequestHandler &cpy)
{
    this->info = cpy.getInfo();
    this->request = cpy.getReq();
    this->method = cpy.getMethod();
    this->response = cpy.getRes();
    this->requestedUrl = cpy.getRequestedUrl();
    this->envp = cpy.getEnvp();
    return (*this);
}

void    RequestHandler::crossRoads(int matchedLocation)
{
    if (this->method == "GET")
        getRequestHandler(matchedLocation);
    else if (this->method == "POST")
        postRequestHandler(matchedLocation);
    else if (this->method == "DELETE")
        deleteRequestHandler(matchedLocation);
}

void RequestHandler::requestFilter(long int matchedLocation)
{
    std::string redirectUrl = matchedLocation >= 0 ? this->info.locations_getter()[matchedLocation].getRedirect() : "null";

    if ((this->requestedUrl == "/" && this->info.getMethod().find(this->method) != std::string::npos)
        || (matchedLocation >= 0 && this->info.locations_getter()[matchedLocation].getMethod().find(this->method) != std::string::npos) || matchedLocation == -1)
    {
        if (redirectUrl != "null")
            this->response = REDIRECT + redirectUrl + "\r\n\r\n";
        else if (matchedLocation == -1 && this->info.getMethod().find(this->method) == std::string::npos)
        {
            this->response = atachStatus(METHOD_NOT_ALLOWED, fileToStr("./view/displayError/method_err.html").c_str());
            return ;
        }            
        else if ((matchedLocation == -1 && this->info.getAutoind() == "true" && this->info.getMethod().find("GET") != std::string::npos)
            || (matchedLocation >= 0 && this->info.locations_getter()[matchedLocation].getAutoind() == "true" && this->info.locations_getter()[matchedLocation].getMethod().find("GET") != std::string::npos))
            this->response = callForAutoindex(matchedLocation);
        else
            crossRoads(matchedLocation);
    }
    else
        this->response = atachStatus(METHOD_NOT_ALLOWED, fileToStr("./view/displayError/method_err.html").c_str());
}

std::string RequestHandler::start(std::string method, std::string requestedUrl, std::vector<std::string> envp)
{
    std::vector<Location>    locations;
    long unsigned int           size = 0;
    long int           matchedLocation = -1;

    this->setMethod(method);
    this->setRequestedUrl(requestedUrl);
    this->envp = envp;
    locations = this->info.locations_getter();
    for (unsigned long i = 0; i < locations.size(); i++)
    {
        if (locations[i].getUrl().length() <= this->requestedUrl.length() && this->requestedUrl.substr(0, locations[i].getUrl().length()) == locations[i].getUrl() && locations[i].getUrl().length() > size)
		{
                //lenght must checked so that /postfile != /postfilekjfsbvfsbsf
				matchedLocation = (long int)i;
				size = locations[i].getUrl().length();
		}
    }
    std::string		completePath = getSearchPath(matchedLocation);
    std::string comp = requestedUrl.find("?") != std::string::npos ? requestedUrl.substr(0, requestedUrl.find("?")) : requestedUrl;
    if (matchedLocation == -1 && comp != "/")
    {
        if (!isDir(completePath.c_str()) && fileExists(completePath.c_str()))
        {
		    return (this->response = atachStatus(SUCCESS,fileToStr(completePath.c_str()).c_str()));

        }
	    else if (isDir(completePath.c_str()))
	    {
	    	if (matchedLocation >= 0 && this->info.locations_getter()[matchedLocation].getIndex() != "null" && fileExists(this->info.locations_getter()[matchedLocation].getIndex().c_str()))
	    		return (this->response = atachStatus(SUCCESS, fileToStr(this->info.locations_getter()[matchedLocation].getIndex().c_str()).c_str()));
	    	else
	    		return (this->response = atachStatus(SUCCESS, fileToStr(WELCOME).c_str()));
	    }
        else if (this->info.getErrPage() != "null" && fileExists(this->info.getErrPage().c_str()))
            return (this->response = atachStatus(NOT_FOUND, fileToStr(this->info.getErrPage().c_str()).c_str()));
        else
            return (this->response = atachStatus(NOT_FOUND, fileToStr("./view/displayError/err.html").c_str()));
    }
    this->requestFilter(matchedLocation);
    return (this->response);
}

RequestHandler::RequestHandler(const RequestHandler &cpy)
{
    *this = cpy;
}

void	RequestHandler::addNewEnvp()
{
	std::stringstream	ss;
	std::string			data;
    unsigned long       urlEnd = this->requestedUrl.find("?");

    if (urlEnd != std::string::npos)
    {
        ss << this->requestedUrl.substr(urlEnd + 1);
	    while (getline(ss, data, '&'))
		    this->envp.push_back(data);
        this->requestedUrl = this->requestedUrl.substr(0, urlEnd);
    }
}



RequestHandler::RequestHandler(ServerConf info, Connection *req): info(info), request(req->getHeader())
{
    this->req = req;
}

RequestHandler::~RequestHandler()
{
}