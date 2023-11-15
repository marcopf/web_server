#ifndef UTILS_HPP
# define UTILS_HPP

#define SUCCESS				"HTTP/1.1 200 OK"
#define BAD_REQUEST			"HTTP/1.1 400 Bad Request"
#define NOT_FOUND			"HTTP/1.1 404 Not Found"
#define	METHOD_NOT_ALLOWED	"HTTP/1.1 405 Method Not Allowed"
#define REDIRECT            "HTTP/1.1 301 Moved Permanently\r\nLocation: "

#define	HTML				"text/html"
#define	PLAIN				"text/plain"

#define	ERR_PAGE			"./view/displayError/err.html"
#define ERR_METHOD			"./view/displayError/method_err.html"
#define	ERR_AUTOINDEX		"./view/displayError/autoindex_err.html"
#define WELCOME				"./view/welcome.html"

#define PYTHON				"/usr/bin/python3" //42mac
#define	PHP					"/usr/bin/php" //LINUX
//#define PHP				"/opt/homebrew/bin/php" //MACOS

#include "../RequestManager/RequestHandler.hpp"
#include "../Socket/Connection.hpp"
class Connection;

unsigned long	getContentLenght(std::string header);
std::string		atachStatus(const char *status, const char *body);
std::string		fileToStr(std::string file);
std::string		findMethod(Connection *req, ServerConf info, std::vector<std::string> envp);
std::string		findUrl(std::string req);
char	        *ft_strnstr(const char *haystack, const char *needle, size_t len);
unsigned long   headerLen(const char *haystack, const char *needle, size_t len);

//CGI UTILS
char			**createMat(std::string command, std::string path, char  *cgiParamater);
void			freeMat(char **mat);
std::string		extensionFinder(std::string path);
char			**convertVector(std::vector<std::string> vect);
std::string		waitAndCheck(int pid, int *fd);
std::string		atachStatusCgi(const char *status, const char *body);
std::string		executeFile(std::string path, std::vector<std::string> envp, char *cgiParameter);


#endif