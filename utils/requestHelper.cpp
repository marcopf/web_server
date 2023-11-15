#include "utils.hpp"

unsigned long getContentLenght(std::string header)
{
	int	pos;
	unsigned long	newLinePos;
	unsigned long	len;

	len = 0;
	pos = header.find("Content-Length");
	if ((unsigned long)pos != std::string::npos)
	{
		newLinePos = header.find("\n", pos);
		if (newLinePos != std::string::npos)
			len = atoi(header.substr((pos + 15), newLinePos - pos).c_str());
	}
	return (len);
}

char	*ft_strnstr(const char *haystack, const char *needle, size_t len)
{
	size_t		i;
	size_t		j;

	i = 0;
	if (*needle == '\0' || needle == NULL)
		return ((char *) haystack);
	if (!haystack && len == 0)
		return (0);
	while (i < len)
	{
		j = 0;
		while (needle[j] == haystack[i + j] && i + j < len)
		{
			if (needle[j + 1] == '\0')
			{
				return ((char *)haystack + i);
			}
			j++;
		}
		i++;
	}
	return (NULL);
}

unsigned long headerLen(const char *haystack, const char *needle, size_t len)
{
	size_t		i;
	size_t		j;

	i = 0;
	if (*needle == '\0' || needle == NULL)
		return (0);
	if (!haystack && len == 0)
		return (0);
	while (i < len)
	{
		j = 0;
		while (needle[j] == haystack[i + j] && i + j < len)
		{
			if (needle[j + 1] == '\0')
			{
				return (i);
			}
			j++;
		}
		i++;
	}
	return (0);
}

std::string atachStatus(const char *status, const char *body)
{
	std::stringstream ss;
	std::string status_s = status, body_s = body;
	ss << "\r\nContent-length: " << body_s.length() << "\r\n\r\n";
	std::string res = status_s + ss.str() + body_s + "\r\n";
	return (res);
}

std::string	fileToStr(std::string file)
{
	std::string str;
	std::ifstream f(file.c_str());
	std::ostringstream ss;
	ss << f.rdbuf();
	str = ss.str();
	f.close();
	return (str);
}

std::string findMethod(Connection *req, ServerConf info, std::vector<std::string> envp)
{
	RequestHandler parser(info, req);

    if (req->getHeader().substr(0, 3) == "GET")
	{
        return (parser.start("GET", findUrl(req->getHeader()), envp));
	}
    else if (req->getHeader().substr(0, 4) == "POST")
    {
		return (parser.start("POST", findUrl(req->getHeader()), envp));
	}
    else if (req->getHeader().substr(0, 6) == "DELETE")
	{
        return (parser.start("DELETE", findUrl(req->getHeader()), envp));
	}
	return (atachStatus("HTTP/1.1 405 Method Not Allowed", fileToStr("./view/displayError/method_err.html").c_str()));
}

std::string findUrl(std::string req)
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