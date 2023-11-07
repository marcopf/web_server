#include "../Socket/RequestHelper.hpp"

char **createMat(std::string command, std::string path)
{
	char **mat;

	mat = (char **)malloc(sizeof(char *) * 3);
	if (command == "./")
		command += path;
	mat[0] = strdup(command.c_str());
	std::cout << command << std::endl;
	mat[1] = strdup(path.c_str());
		std::cout << path << std::endl;
	mat[2] = 0;
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

	ret = (char **)malloc(sizeof(char *) * vect.size() + 1);
	ret[vect.size()] = 0;
	for (unsigned long i = 0; i < vect.size(); i++)
		ret[i] = strdup(vect[i].c_str());
	return (ret);
}

std::string	waitAndCheck(int pid, int *fd)
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
		return (close(fd[0]), fileToStr("./view/cgi_error.html"));
	return ("OK");
}

std::string	RequestHelper::executeFile(std::string path, std::vector<std::string> envp)
{
	std::string		executed, result, command = extensionFinder(path);
	int				fd[2], pid;
	char			**mat;
	char			c;

	if (pipe(fd) == -1)
		return ("null");
	pid = fork();
	if (pid == 0)
	{
		mat = createMat(command, path);
		close(fd[0]);
		dup2(fd[1], 1);
		if (execve(mat[0], mat, convertVector(envp)) == -1)
			return ("null");
	}
	result = waitAndCheck(pid, fd);
	if (result != "OK")
		return RequestHelper::atachStatus(NOT_FOUND, HTML, result.c_str());
	while (read(fd[0], &c, 1))
		executed += c;
	close(fd[0]);
	return atachStatus(SUCCESS, HTML, executed.c_str());
}