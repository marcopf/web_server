#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include "../parser/ServerConf.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

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

//#define PYTHON				"/usr/bin/python3"
#define PYTHON				"/usr/local/bin/python3" //42mac
#define	PHP					"/usr/bin/php" //LINUX
//#define PHP					"/opt/homebrew/bin/php" //MACOS

class RequestHandler
{
private:
    ServerConf  info;
    std::string request;
    std::string method;
    std::string requestedUrl;
    std::string response;
public:
    std::string getReq() const;
    std::string getRes() const;
    std::string getMethod() const;
    std::string getRequestedUrl() const;
    ServerConf  getInfo() const;
    void        setMethod(std::string method);
    void        setRequestedUrl(std::string url);
    std::string start(std::string method, std::string requestedUrl);
    void        requestFilter(long int matchedLocation);
    std::string callForAutoindex(int i);
    std::string getSearchPath(int i);
    void        crossRoads();
    void        getRequestHandler();
    void        postRequestHandler();
    void        deleteRequestHandler();
    std::string autoindex(std::string directoryPath, int i);
    RequestHandler &operator=(const RequestHandler &cpy);
    RequestHandler(const RequestHandler &cpy);
    RequestHandler(ServerConf info, std::string req);
    ~RequestHandler();
};

bool		isDir(const char *path);
bool		isFile(const char *path);
bool 		fileExists(const char* fileName);
void		freeMat(char **mat);
char 		**createMat(std::string command, std::string path);
std::string	fileToStr(std::string file);
std::string	getCurrentRoot(ServerConf serv, int i);
std::string atachStatus(const char *status, const char *type, const char *body);

#endif