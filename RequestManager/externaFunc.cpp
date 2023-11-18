#include "RequestHandler.hpp"

/**
 * The function "createMat" creates a matrix of strings based on the given command, path, and CGI
 * parameter.
 * 
 * @param command The "command" parameter is a string that represents a command to execute
 * executable file. It is used to create the first element of the "mat" array.
 * @param path The "path" parameter is a string that represents the path to a file or directory.
 * @param cgiParamater The `cgiParamater` parameter is a pointer to a character array that represents
 * the CGI parameter. It is used to append additional information to the `path` parameter when creating
 * the `mat` array.
 * 
 * @return a pointer to a 2D array of characters (char **).
 */
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

/**
 * The function "freeMat" frees the memory allocated for a 2D array of characters.
 * 
 * @param mat The parameter "mat" is a pointer to a pointer to a character, which represents a 2D array
 * of characters.
 */
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

/**
 * The function "extensionFinder" takes a file path as input and returns the file extension if it is
 * either ".py" or ".php", otherwise it returns "./".
 * 
 * @param path The "path" parameter is a string that represents the file path or file name.
 * 
 * @return The function `extensionFinder` returns a string representing the file extension of the given
 * `path`. If the file extension is ".py", it returns "PYTHON". If the file extension is ".php", it
 * returns "PHP". Otherwise, it returns "./".
 */
std::string	extensionFinder(std::string path)
{
	unsigned long pos = path.find_last_of(".");

	if (pos != std::string::npos && path.substr(pos) == ".py")
		return (PYTHON);
	if (pos != std::string::npos && path.substr(pos) == ".php")
		return (PHP);
	return ("./");
}

/**
 * The function "convertVector" takes a vector of strings and converts it into a dynamically allocated
 * array of C-style strings.
 * 
 * @param vect The parameter "vect" is a vector of strings.
 * 
 * @return a pointer to a pointer of characters (char **).
 */
char	**convertVector(std::vector<std::string> vect)
{
	char	**ret;

	ret = (char **)malloc(sizeof(char *) * (vect.size() + 1));
	ret[vect.size()] = 0;
	for (unsigned long i = 0; i < vect.size(); i++)
		ret[i] = strdup(vect[i].c_str());
	return (ret);
}

/**
 * The function "fileToStr" reads the contents of a file and returns it as a string.
 * 
 * @param file The "file" parameter is a string that represents the file path of the file you want to
 * read.
 * 
 * @return The function `fileToStr` returns a string that contains the contents of the file specified
 * by the `file` parameter.
 */
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

/**
 * The function checks if a file with the given name exists.
 * 
 * @param fileName A C-style string representing the name of the file to check for existence.
 * 
 * @return a boolean value. It returns true if the file with the given fileName exists, and false
 * otherwise.
 */
bool fileExists(const char* fileName) {
    std::ifstream file(fileName);
    return file.good();
}

/**
 * The function returns the root directory path of a server configuration or a specific location within
 * the server configuration root path .
 * 
 * @param serv The `serv` parameter is an object of type `ServerConf`.
 * @param i The parameter "i" is an integer that represents the index of the location in the server
 * configuration. It is used to access a specific location in the server configuration.
 * 
 * @return The function `getCurrentRoot` returns a `std::string` value.
 */
std::string	getCurrentRoot(ServerConf serv, int i)
{
	if (i > -1 && serv.locations_getter()[i].getRoot() != "null")
		return (serv.locations_getter()[i].getRoot());
	return (serv.getRoot());
}

/**
 * The function checks if a given path is a directory or not.
 * 
 * @param path The path parameter is a string that represents the directory path that you want to
 * check.
 * 
 * @return The function isDir returns a boolean value. It returns true if the given path is a
 * directory, and false if it is not a directory or if there was an error opening the directory.
 */
bool	isDir(const char *path)
{
	DIR*	temp;

	temp = opendir(path);
	if (temp == NULL)
		return (false);
	closedir(temp);
	return (true);
}

/**
 * The function checks if a given path is a file.
 * 
 * @param path The path parameter is a string that represents the file path.
 * 
 * @return a boolean value. If the given path is a file (not a directory) and the file exists, then the
 * function returns true. Otherwise, it returns false.
 */
bool	isFile(const char *path)
{
	if (!isDir(path) && fileExists(path))
		return (true);
	return (false);	
}