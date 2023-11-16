#include "RequestHandler.hpp"

ServerConf RequestHandler::getInfo() const
{
    return (this->info);
}

std::string RequestHandler::getReq() const
{
    return (this->request);
}
char *RequestHandler::getRes() const
{
    return (this->response);
}
unsigned long   RequestHandler::getResSize() const
{
    return (this->resSize);
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
    this->resSize = cpy.resSize;
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
            this->response = (char *)(REDIRECT + redirectUrl + "\r\n\r\n").c_str();
        else if (matchedLocation == -1 && this->info.getMethod().find(this->method) == std::string::npos)
        {
            atachStatus(METHOD_NOT_ALLOWED, "./view/displayError/method_err.html", "");
            return ;
        }            
        else if ((matchedLocation == -1 && this->info.getAutoind() == "true" && this->info.getMethod().find("GET") != std::string::npos)
            || (matchedLocation >= 0 && this->info.locations_getter()[matchedLocation].getAutoind() == "true" && this->info.locations_getter()[matchedLocation].getMethod().find("GET") != std::string::npos))
            callForAutoindex(matchedLocation);
        else
            crossRoads(matchedLocation);
    }
    else
        atachStatus(METHOD_NOT_ALLOWED, "./view/displayError/method_err.html", "");
}

void RequestHandler::start(std::string method, std::string requestedUrl, std::vector<std::string> envp)
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
		    return (atachStatus(SUCCESS, completePath.c_str(), ""));

        }
	    else if (isDir(completePath.c_str()))
	    {
	    	if (matchedLocation >= 0 && this->info.locations_getter()[matchedLocation].getIndex() != "null" && fileExists(this->info.locations_getter()[matchedLocation].getIndex().c_str()))
	    		return (atachStatus(SUCCESS, this->info.locations_getter()[matchedLocation].getIndex().c_str(), ""));
	    	else
	    		return (atachStatus(SUCCESS, WELCOME, ""));
	    }
        else if (this->info.getErrPage() != "null" && fileExists(this->info.getErrPage().c_str()))
            return (atachStatus(NOT_FOUND, this->info.getErrPage().c_str(), ""));
        else
            return (atachStatus(NOT_FOUND, "./view/displayError/err.html", ""));
    }
    this->requestFilter(matchedLocation);
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

void RequestHandler::findMethod()
{
    if (req->getHeader().substr(0, 3) == "GET")
	{
        this->start("GET", findUrl(this->req->getHeader()), envp);
	}
    else if (req->getHeader().substr(0, 4) == "POST")
    {
		this->start("POST", findUrl(this->req->getHeader()), envp);
	}
    else if (req->getHeader().substr(0, 6) == "DELETE")
	{
        this->start("DELETE", findUrl(this->req->getHeader()), envp);
	}
    else
	    atachStatus("HTTP/1.1 405 Method Not Allowed", "./view/displayError/method_err.html", "");
}

void RequestHandler::atachStatus(const char *status, const char *fileName, std::string body)
{
	std::stringstream ss;
	std::string status_s = status;
    int i, j;

    if (body == "")
    {
        std::ifstream file(fileName, std::ios::binary);
        file.seekg(0, std::ios::end);
        std::streampos fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        char *buffer = new char [fileSize];
        file.read(buffer, fileSize);
        file.close();
        ss << status << "\r\nContent-length: " << fileSize << "\r\n\r\n";
        if (this->response)
            delete [] this->response;
        this->response = new char [ss.str().length() + fileSize + 2];
        this->resSize = ss.str().length() + fileSize + 2;
        for (i = 0; i < ss.str().length(); i++)
            this->response[i] = ss.str()[i];
        for (j = 0; j < fileSize; j++)
            this->response[i + j] = buffer[j];
        this->response[i + j] = '\r';
        this->response[i + j + 1] = '\n';
        delete [] buffer;
    }
    else
    {
        ss << status << "\r\nContent-length: " << body.length() << "\r\n\r\n" << body << "\r\n";
        this->response = new char [ss.str().length()];
        for (i = 0; i < ss.str().length(); i++)
            this->response[i] = ss.str()[i];
    }
}

RequestHandler::RequestHandler(ServerConf info, Connection *req, std::vector<std::string> envp, bool bodySizeExeeded): info(info), request(req->getHeader())
{
    this->envp = envp;
    this->req = req;
    this->response = 0;
    if (bodySizeExeeded)
    {
        atachStatus("HTTP/1.1 413 Request Entity Too Large", "./view/displayError/err.html", "");
        return ;
    }
    this->findMethod();
}

RequestHandler::~RequestHandler()
{
    if (this->response)
        delete [] this->response;
}