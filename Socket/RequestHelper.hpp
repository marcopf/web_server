#ifndef REQUESTHELPER_HPP
#define REQUESTHELPER_HPP

#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include "../parser/ServerConf.hpp"
#include "../RequestManager/RequestHandler.hpp"
#include <stdlib.h>


#define SUCCESS				"HTTP/1.1 200 OK"
#define BAD_REQUEST			"HTTP/1.1 400 Bad Request"
#define NOT_FOUND			"HTTP/1.1 404 Not Found"
#define	METHOD_NOT_ALLOWED	"HTTP/1.1 405 Method Not Allowed"
#define REDIRECT            "HTTP/1.1 301 Moved Permanently\r\nLocation: "

#define	HTML				"text/html"
#define	PLAIN				"text/plain"

#define	ERR_PAGE			"./view/err.html"
#define ERR_METHOD			"./view/method_err.html"
#define	ERR_AUTOINDEX		"./view/autoindex_err.html"
#define WELCOME				"./view/welcome.html"

#define PYTHON				"/usr/bin/python3" //42mac
#define	PHP					"/usr/bin/php" //LINUX
//#define PHP					"/opt/homebrew/bin/php" //MACOS


class RequestHelper
{
private:
    /* data */
public:
    static std::string fileToStr(std::string str);
    static unsigned long getContentLenght(std::string str);
    static std::string atachStatus(const char *status, const char *type, const char *body);
    static std::string findUrl(std::string req);
    static std::string findMethod(std::string str, ServerConf info, std::vector<std::string> envp);
    static std::string executeFile(std::string path, std::vector<std::string> envp);
    RequestHelper(/* args */);
    ~RequestHelper();
};


#endif