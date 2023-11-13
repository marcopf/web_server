#include "ServerConf.hpp"
#include <fstream>

int	ServerConf::getIntPort() const
{
	return (atoi(this->port.c_str()));
}
int	ServerConf::getIntMbs() const
{
	return (atoi(this->max_body_size.c_str()));
}

std::string	ServerConf::getPort() const
{
	return (this->port);
}

std::string	ServerConf::getRoot(void) const
{
	return (this->root);
}

std::string		ServerConf::getListen() const
{
	return (this->listen);
}

std::string		ServerConf::getPath() const
{
	return (this->path);
}

std::string		ServerConf::getHost() const
{
	return (this->host);
}

std::string		ServerConf::getServerName() const
{
	return (this->server_name);
}

std::string		ServerConf::getErrPage() const
{
	return (this->err_page);
}
std::string		ServerConf::getAutoind() const
{
	return (this->autoind);
}

std::string	ServerConf::getMaxBodySize() const
{
	return (this->max_body_size);
}

std::string	ServerConf::getMethod() const
{
	return (this->method);
}

std::string	ServerConf::getIndex() const
{
	return (this->index);
}

std::vector<int>	ServerConf::getPorts() const
{
	std::stringstream 			ss(this->port);
	std::string					data;
	std::vector<int>	ports;
	bool				isAlreadyIn = false;

	while (getline(ss, data, ' '))
	{
		for (unsigned long  i = 0; i < ports.size(); i++)
		{
			if (ports[i] == atoi(data.c_str()))
			isAlreadyIn = true;
		}
		if (!isAlreadyIn)
			ports.push_back(atoi(data.c_str()));
		isAlreadyIn = false;
	}
	return (ports);
}

const ServerConf	&ServerConf::operator=(const ServerConf &cpy)
{
	this->listen = cpy.getListen();
	this->port = cpy.getPort();
	this->host = cpy.getHost();
	this->server_name = cpy.getServerName();
	this->err_page = cpy.getErrPage();
	this->max_body_size = cpy.getMaxBodySize();
	this->root = cpy.getRoot();
	this->index = cpy.getIndex();
	this->file = cpy.getFile();
	this->locations = cpy.locations_getter();
	this->autoind = cpy.getAutoind();
	this->method = cpy.getMethod();
	this->path = cpy.getPath();
	return (*this);
}

ServerConf::ServerConf(const ServerConf &cpy)
{
	*this = cpy;
}

std::string	getLineKeyServ(std::string line)
{
	std::string sub = line.substr(0, line.find(" "));
	std::string key_list [] = {"location", "listen", "host", "port", "server_name", "err_page", "max_body_size", "root", "autoind", "index", "path", "url", "method", "extension", "redirect", "cgi-bin"};

	for (int i = 0; i < 16; i++)
	{
		if (sub.find(key_list[i]) != std::string::npos)
			return (key_list[i]);
	}
	return ("null");
}

std::string	ServerConf::getValP(std::string file, std::string key)
{
	std::stringstream	ss(file);
	std::string			sub;
	std::string			data;

	while (getline(ss, data, '\n'))
	{
		if (key != getLineKeyServ(data))
			continue ;
		if (data.find(key) != std::string::npos && data.find(" ") != std::string::npos)
		{
			sub = data.substr((data.find(key) + key.length()) + 1, data.length() - (data.find(key) + key.length()) - 2);
			for (unsigned int i = 0; i < sub.length(); i++)	
			{
				if (!isspace(sub[i]))
					return (data.substr(data.find(" ") + 1, data.length() - (data.find(key) + key.length()) - 2));
			}
			return ("null");
		}
		else if (data.find(key) != std::string::npos && data.find(";") != std::string::npos)
			return (data.substr((data.find(key) + key.length()), data.find(";") - 1));
	}
	return ("null");
}

void	appendhost(std::string str)
{
	std::ofstream file("/etc/hosts", std::ios_base::app);
	std::ifstream inp_file("/etc/hosts");
	std::string	data;

	if (!file.is_open())
	{
		std::cerr << YELLOW << "cannot open /etc/hosts try again with" << RED << " SUDO" << YELLOW << "..." << std::endl;
		return ;
	}
	while (getline(inp_file, data, '\n'))
	{
		if (data.find("127.0.0.1	" + str) != std::string::npos)
		{
			file.close();
			return ;
		}
	}
	file << "127.0.0.1	" << str << "\n";
	file.close();
}

void	splitHost(std::string hosts)
{
	std::stringstream s_hosts;
	std::string data;

	s_hosts << hosts;
	while (getline(s_hosts, data, ' '))
		appendhost(data);
}

void	ServerConf::parse(void)
{
	this->listen = getValP(this->file, "listen");
	this->port = getValP(this->file, "port");
	this->host = getValP(this->file, "host");
	this->server_name = getValP(this->file, "server_name");
	if(this->server_name != "null")
		splitHost(this->server_name);
	this->err_page = getValP(file, "err_page");
	this->max_body_size = getValP(this->file, "max_body_size");
	this->root = getValP(this->file, "root");
	this->index = getValP(this->file, "index");
	this->autoind = getValP(this->file, "autoind");
	this->method = getValP(this->file, "method");
	this->path = getValP(this->file, "path");
}

void	ServerConf::getInfo(void)
{
	std::cout << "--SERVER_CONF--" << std::endl; 
	std::cout << "port: " << this->port << "\nlisten: " << this->listen << "\nserver_name: " << this->server_name << "\nhost: " << this->host << "\nerr_page: " << this->err_page << "\nmax_body_size: " << this->max_body_size << "\nroot: " << this->root << "\nindex: " << this->index << "\nautoind: " << this->autoind << "\nmethod: " << this->method<< "\npath: " << this->path << std::endl;
	std::cout << "---------------\n" << std::endl; 
}

std::string	ServerConf::getFile(void) const
{
	return (this->file);
}

ServerConf::ServerConf(void)
{
	this->listen = "";
	this->port = "";
	this->host = "";
	this->server_name = "";
	this->err_page = "";
	this->max_body_size = "";
	this->root = "";
	this->index = "";
	this->autoind = "";
	this->method = "";
}

std::vector<Location>	ServerConf::locations_getter(void) const
{
	return (this->locations);
}

std::vector<std::string>	ServerConf::getLocations(void)
{
	int counter, counter2;
	std::string	target = "location";
	std::string toAdd;
	std::vector<std::string> location_list;

	counter = this->file.find(target);
	while ((unsigned long)counter != std::string::npos)
	{
		counter2 = this->file.find(target, counter + target.length());
		if ((unsigned long)counter2 == std::string::npos)
		{
			toAdd = this->file.substr(counter);
			location_list.push_back(toAdd);
			break;
		}
		toAdd = this->file.substr(counter, counter2 - counter);
		location_list.push_back(toAdd);
		counter = counter2;
	}
	return (location_list);
}

ServerConf::ServerConf(std::string file): file(file)
{
	std::vector<std::string> list;

	this->parse();
	list = this->getLocations();
	for (unsigned int i = 0; i < list.size(); i++)
	{
		this->locations.push_back(Location(list[i]));
	}
}

ServerConf::~ServerConf()
{
}