#ifndef REQUESTHELPER_HPP
#define REQUESTHELPER_HPP

#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include "../parser/ServerConf.hpp"
#include "RequestHandler.hpp"
#include <stdlib.h>

class RequestHelper
{
private:
    /* data */
public:
    static std::string fileToStr(std::string str);
    static unsigned long getContentLenght(std::string str);
    static std::string atachStatus(const char *status, const char *type, const char *body);
    static std::string findUrl(std::string req);
    static std::string findMethod(std::string str, ServerConf info);
    RequestHelper(/* args */);
    ~RequestHelper();
};


#endif