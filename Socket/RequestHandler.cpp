#include "RequestHandler.hpp"
#include "RequestHelper.hpp"
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
    return (*this);
}

void    RequestHandler::crossRoads()
{
    if (this->method == "GET")
        getRequestHandler();
    else if (this->method == "POST")
        postRequestHandler();
    else if (this->method == "DELETE")
        deleteRequestHandler();
}

void RequestHandler::requestFilter(long int matchedLocation)
{
    std::string redirectUrl = matchedLocation >= 0 ? this->info.locations_getter()[matchedLocation].getRedirect() : "null";
    std::cout << redirectUrl << std::endl;

    if ((this->requestedUrl == "/" && this->info.getMethod().find(this->method) != std::string::npos)
        || (matchedLocation >= 0 && this->info.locations_getter()[matchedLocation].getMethod().find(this->method) != std::string::npos))
    {
        if (redirectUrl != "null")
        {
            this->response = REDIRECT + redirectUrl + "\r\n\r\n";
            std::cout << this->response << std::endl;
        }
        else if ((matchedLocation == -1 && this->info.getAutoind() == "true" && this->info.getMethod().find("GET") != std::string::npos)
            || (matchedLocation >= 0 && this->info.locations_getter()[matchedLocation].getAutoind() == "true" && this->info.locations_getter()[matchedLocation].getMethod().find("GET") != std::string::npos))
            this->response = callForAutoindex(matchedLocation);
        else
            crossRoads();
    }
    else
        this->response = RequestHelper::atachStatus(METHOD_NOT_ALLOWED, "text/html", RequestHelper::fileToStr("./view/method_err.html").c_str());
}

std::string RequestHandler::start(std::string method, std::string requestedUrl)
{
    std::vector<Location>    locations;
    long unsigned int           size = 0;
    long int           matchedLocation = -1;

    this->setMethod(method);
    this->setRequestedUrl(requestedUrl);
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
    this->requestFilter(matchedLocation);
    return (this->response);
}

RequestHandler::RequestHandler(const RequestHandler &cpy)
{
    *this = cpy;
}

void    RequestHandler::getRequestHandler()
{
    this->response = RequestHelper::atachStatus(METHOD_NOT_ALLOWED, "text/html", RequestHelper::fileToStr("./view/welcome.html").c_str());
}

void    RequestHandler::postRequestHandler()
{
    this->response = RequestHelper::atachStatus(METHOD_NOT_ALLOWED, "text/html", RequestHelper::fileToStr("./view/temp_err.html").c_str());
}

void    RequestHandler::deleteRequestHandler()
{
    this->response = RequestHelper::atachStatus(METHOD_NOT_ALLOWED, "text/html", RequestHelper::fileToStr("./view/temp_err.html").c_str());
}

RequestHandler::RequestHandler(ServerConf info, std::string req): info(info), request(req)
{
}

RequestHandler::~RequestHandler()
{
}