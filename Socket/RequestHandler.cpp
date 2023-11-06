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

void RequestHandler::requestFilter(long int matchedLocation)
{
    // if ((this->requestedUrl == "/" && this->info.getMethod().find(this->method) != std::string::npos)
    //     || (matchedLocation >= 0 && this->info.locations_getter()[matchedLocation].getMethod().find(this->method) != std::string::npos))
    // {
    if ((matchedLocation == -1 && this->info.getAutoind() == "true" && this->info.getMethod().find("GET") != std::string::npos)
        || (matchedLocation >= 0 && this->info.locations_getter()[matchedLocation].getAutoind() == "true") && this->info.locations_getter()[matchedLocation].getMethod().find("GET") != std::string::npos)
            this->response = callForAutoindex(matchedLocation);
    // }
    else
        this->response = RequestHelper::atachStatus("HTTP/1.1 200 OK", "text/html", RequestHelper::fileToStr("./view/method_err.html").c_str());
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
        std::cout << "Hey non ho trovato nulla!" << std::endl;
    return (this->response);
}

RequestHandler::RequestHandler(const RequestHandler &cpy)
{
    *this = cpy;
}

RequestHandler::RequestHandler(ServerConf info, std::string req): info(info), request(req)
{
}

RequestHandler::~RequestHandler()
{
}