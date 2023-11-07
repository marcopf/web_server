#include "RequestHelper.hpp"


unsigned long	RequestHelper::getContentLenght(std::string header)
{
	int	pos;
	unsigned long	newLinePos;
	unsigned long	len;

	len = 0;
	pos = header.find("Content-Length");
	if ((unsigned long)pos != std::string::npos)
	{
		newLinePos = header.find("\n", pos);
		len = atoi(header.substr((pos + 15), newLinePos - pos).c_str());
	}
	return (len);
}

std::string RequestHelper::atachStatus(const char *status, const char *type, const char *body)
{
	std::stringstream ss;
	std::string status_s = status, body_s = body;
	ss << "\r\nContent-length: " << body_s.length() << "\r\nContent-Type: " << type << "\r\n\r\n";
	std::string res = status_s + ss.str() + body_s + "\r\n";
	return (res);
}

std::string	RequestHelper::fileToStr(std::string file)
{
	std::string str;
	std::ifstream f(file.c_str());
	std::ostringstream ss;
	ss << f.rdbuf();
	str = ss.str();
	f.close();
	return (str);
}

std::string RequestHelper::findMethod(std::string req, ServerConf info)
{
	RequestHandler parser(info, req);

    if (req.substr(0, 3) == "GET")
	{
        return (parser.start("GET", findUrl(req)));
	}
    else if (req.substr(0, 4) == "POST")
    {
		return (parser.start("POST", findUrl(req)));
	}
    else if (req.substr(0, 6) == "DELETE")
	{
        return (parser.start("DELETE", findUrl(req)));
	}
	return (RequestHelper::atachStatus("HTTP/1.1 200 OK", "text/html", RequestHelper::fileToStr("./../view/method_err.html").c_str()));
}

std::string RequestHelper::findUrl(std::string req)
{
	unsigned long firstSpace;
	unsigned long secondSpace;

	firstSpace = req.find(" ");
	if (firstSpace != std::string::npos)
		secondSpace = req.find(" ", firstSpace + 1);
	if (secondSpace != std::string::npos)
		return (req.substr(firstSpace + 1, secondSpace - firstSpace - 1));
	return ("NULL");
}

RequestHelper::RequestHelper(/* args */)
{
}

RequestHelper::~RequestHelper()
{
}