#include "RequestHandler.hpp"


/**
 * The function `waitAndCheck` waits for a child process to finish and checks if it exited normally or
 * encountered an error.
 * 
 * @param pid The `pid` parameter is the process ID of the child process that the `waitAndCheck`
 * function is waiting for.
 * @param fd The parameter `fd` is a pointer to an integer. It is used to pass a file descriptor to the
 * function `waitAndCheck`. The function uses this file descriptor to close the write end of a pipe
 * (`fd[1]`) after it is done with it.
 * 
 * @return a std::string.
 */
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

/**
 * The function `executeFile` executes a file specified by the `path` parameter, using the appropriate
 * command based on the file extension, and returns the result.
 * 
 * @param path The `path` parameter is a string that represents the file path of the file to be
 * executed.
 * @param envp The `envp` parameter is a vector of strings that represents the environment variables
 * for the executed file. Each string in the vector should be in the format "variable=value". These
 * environment variables will be passed to the executed file when using the `execve` function.
 * @param cgiParameter The `cgiParameter` parameter is a character pointer that represents the CGI
 * parameter. It is used in the `createMat` function to create the matrix `mat` which is then passed to
 * the `execve` function. The `execve` function is responsible for executing the CGI script specified
 * by
 * 
 * @return The function does not have a return type specified, so it is likely returning void.
 */
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