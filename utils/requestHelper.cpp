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