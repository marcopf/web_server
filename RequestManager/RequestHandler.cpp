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

/**
 * The crossRoads function determines the appropriate request handler based on the HTTP method.
 * 
 * @param matchedLocation The parameter "matchedLocation" is an integer that represents the location or
 * endpoint that was matched in the request. It is used to determine which specific request handler
 * function should be called based on the HTTP method (GET, POST, DELETE) specified in the request.
 */
void    RequestHandler::crossRoads(int matchedLocation)
{
    if (this->method == "GET")
        getRequestHandler(matchedLocation);
    else if (this->method == "POST")
        postRequestHandler(matchedLocation);
    else if (this->method == "DELETE")
        deleteRequestHandler(matchedLocation);
}

/**
 * The function `requestFilter` handles requests by checking if the requested URL and method match the
 * specified criteria, and redirects or calls other functions accordingly.
 * 
 * @param matchedLocation The parameter `matchedLocation` is of type `long int` and represents the
 * index of a matched location in a collection or array. It is used to determine if a location has been
 * matched or not. If `matchedLocation` is greater than or equal to 0, it means a location has
 * 
 * @return The function does not have a return type specified, so it does not return anything.
 */
void RequestHandler::requestFilter(long int matchedLocation)
{
    std::string redirectUrl = matchedLocation >= 0 ? this->info.locations_getter()[matchedLocation].getRedirect() : "null";

    if ((this->requestedUrl == "/" && this->info.getMethod().find(this->method) != std::string::npos)
        || (matchedLocation >= 0 && this->info.locations_getter()[matchedLocation].getMethod().find(this->method) != std::string::npos) || matchedLocation == -1)
    {
        if (redirectUrl != "null")
            this->response = strdup((char *)(REDIRECT + redirectUrl + "\r\n\r\n").c_str());
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

/**
 * The start function handles incoming requests by matching the requested URL with configured locations
 * and performing appropriate actions based on the match.
 * 
 * @param method The method parameter is a string that represents the HTTP method used in the request
 * (e.g., GET, POST, PUT, DELETE, etc.).
 * @param requestedUrl The `requestedUrl` parameter is a string that represents the URL that was
 * requested by the client. It is used to determine which location in the server configuration matches
 * the requested URL.
 * @param envp The `envp` parameter is a vector of strings that represents the environment variables
 * passed to the request handler. These variables contain information about the current execution
 * environment, such as the server's configuration settings, the client's IP address, and other
 * relevant information.
 * 
 * @return the result of the `atachStatus` function.
 */
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
            else if (matchedLocation == -1 && this->info.getIndex() != "null" && fileExists(this->info.getIndex().c_str()))
                return (atachStatus(SUCCESS, this->info.getIndex().c_str(), ""));
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

/**
 * The function "addNewEnvp" extracts query parameters from a URL and adds them to a vector called
 * "envp".
 */
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

/**
 * The function `findMethod()` checks the HTTP method in the request header and calls the appropriate
 * method based on the method type.
 */
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

/**
 * The function `atachStatus` attaches a status, file, and body to a response message.
 * 
 * @param status The `status` parameter is a C-style string that represents the HTTP status code and
 * status message. For example, "200 OK" or "404 Not Found".
 * @param fileName The `fileName` parameter is a string that represents the name of the file from which
 * the response body will be read.
 * @param body The "body" parameter is a string that represents the body of the HTTP response. It can
 * contain any content that you want to include in the response, such as HTML, JSON, or plain text.
 */
void RequestHandler::atachStatus(const char *status, const char *fileName, std::string body)
{
	std::stringstream   ss;
	std::string         status_s = status;
    unsigned long       i;
    long                j;

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
        {
            delete [] this->response;
            this->response = 0;
        }
        this->response = new char [ss.str().length() + fileSize + 3];
        this->resSize = ss.str().length() + fileSize + 2;
        for (i = 0; i < ss.str().length(); i++)
            this->response[i] = ss.str()[i];
        for (j = 0; j < fileSize; j++)
            this->response[i + j] = buffer[j];
        this->response[i + j] = '\r';
        this->response[i + j + 1] = '\n';
        this->response[i + j + 2] = '\0';
        delete [] buffer;
    }
    else
    {
        if (this->response)
        {
            delete [] this->response;
            this->response = 0;
        }
        ss << status << "\r\nContent-length: " << body.length() << "\r\n\r\n" << body << "\r\n";
        this->response = new char [ss.str().length() + 1];
        this->resSize = ss.str().length() + 1;
        for (i = 0; i < ss.str().length(); i++)
            this->response[i] = ss.str()[i];
        this->response[i] = 0;
    }
}

/**
 * The RequestHandler constructor initializes the RequestHandler object with server configuration
 * information, connection header, environment variables, and checks if the request body size has
 * exceeded the limit.
 * 
 * @param info The `info` parameter is of type `ServerConf` and contains information about the server
 * configuration.
 * @param req A pointer to a Connection object that represents the incoming request.
 * @param envp envp is a vector of strings that contains the environment variables for the request.
 * These variables provide information about the server and the current request, such as the server's
 * hostname, the request method, and the request URI.
 * @param bodySizeExeeded A boolean value indicating whether the size of the request body exceeds the
 * allowed limit.
 * 
 * @return If the bodySizeExeeded flag is true, the function will return without setting any response.
 * Otherwise, it will call the findMethod() function.
 */
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
    {
        delete [] this->response;
        this->response = 0;
    }
}