#include "RequestHandler.hpp"

char **createMat(std::string command, std::string path, char  *cgiParamater)
{
	char **mat;
	mat = (char **)malloc(sizeof(char *) * 3);
	if (command == "./")
		command += path;
	mat[0] = strdup(command.c_str());
	if (cgiParamater)
		mat[1] = strdup((path + " " + std::string(cgiParamater)).c_str());
	else
		mat[1] = strdup(path.c_str());
	mat[2] = NULL;

	return (mat);
}

void	freeMat(char **mat)
{
	int	i;

	i = 0;
	while (mat[i])
	{
		free(mat[i]);
		mat[i] = 0;
		i++;
	}
	free(mat);
	mat = 0;
}

//RETURN EXECUTER PROGRAM ABSOLUTE PATH AFTER EXTENSION IS CHECKED
std::string	extensionFinder(std::string path)
{
	unsigned long pos = path.find_last_of(".");

	if (pos != std::string::npos && path.substr(pos) == ".py")
		return (PYTHON);
	if (pos != std::string::npos && path.substr(pos) == ".php")
		return (PHP);
	return ("./");
}

char	**convertVector(std::vector<std::string> vect)
{
	char	**ret;

	ret = (char **)malloc(sizeof(char *) * (vect.size() + 1));
	ret[vect.size()] = 0;
	for (unsigned long i = 0; i < vect.size(); i++)
		ret[i] = strdup(vect[i].c_str());
	return (ret);
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

std::string	RequestHandler::waitAndCheck(int pid, int *fd)
{
	std::time_t	start = time(0);
	std::time_t	newTime = time(0);
	int			stat;
	int 		flag = 0;

	while (waitpid(pid, &stat, WNOHANG | WUNTRACED) == 0)
	{
		newTime = time(0);
		if (newTime - start >= 3)
		{
			kill(pid, 9);
			flag = 1;
			break ;
		}
		sleep(1);
	}
	close(fd[1]);
	if (flag || (WIFEXITED(stat) && WEXITSTATUS(stat)))
		return (close(fd[0]), fileToStr("./view/displayError/cgi_error.html"));
	return ("OK");
}

std::string atachStatusCgi(const char *status, const char *body)
{
	std::stringstream ss;
	std::string status_s = status, body_s = body;
	ss << "\r\nContent-length: " << body_s.length() << "\r\n" << "Content-Type: text/html"  << "\r\n\r\n";
	std::string res = status_s + ss.str() + body_s + "\r\n";
	return (res);
}

void	RequestHandler::executeFile(std::string path, std::vector<std::string> envp, char *cgiParameter)
{
	std::string		executed, result, command = extensionFinder(path);
	int				fd[2], pid;
	char			**mat;
	char			c;

	if (pipe(fd) == -1)
		return ;
	pid = fork();
	if (pid == 0)
	{
		mat = createMat(command, path, cgiParameter);
		close(fd[0]);
		dup2(fd[1], 1);
		if (execve(mat[0], mat, convertVector(envp)) == -1)
			return ;
	}
	result = waitAndCheck(pid, fd);
	if (result != "OK")
	{
		atachStatus(NOT_FOUND, 0, result.c_str());
		return ;
	}
	while (read(fd[0], &c, 1))
		executed += c;
	close(fd[0]);
	atachStatus(SUCCESS, 0, executed.c_str());
}