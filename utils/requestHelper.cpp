#include "utils.hpp"

/**
 * The function `getContentLength` returns the length of the body in a given HTTP header.
 * 
 * @param header The parameter "header" is a string that represents the HTTP header of a response.
 * 
 * @return the content length as an unsigned long integer.
 */
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

/**
 * The function `ft_strnstr` searches for the first occurrence of a substring `needle` in a string
 * `haystack` within a specified length `len`.
 * 
 * @param haystack A pointer to the string in which to search for the needle.
 * @param needle The `needle` parameter is a pointer to the substring that we are searching for within
 * the `haystack` string.
 * @param len The parameter "len" is of type "size_t" and represents the maximum number of characters
 * to search in the "haystack" string.
 * 
 * @return The function `ft_strnstr` returns a pointer to the first occurrence of the `needle` string
 * within the `haystack` string, up to a maximum of `len` characters. If the `needle` string is empty
 * or `NULL`, it returns a pointer to the `haystack` string. If the `haystack` string is `NULL` and
 * `len` is
 */
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

/**
 * The function "headerLen" calculates the length of a header string within a given haystack string, up
 * to a specified length.
 * 
 * @param haystack The `haystack` parameter is a pointer to a character array (string) that represents
 * the larger string in which we want to search for the `needle` string.
 * @param needle The needle parameter is a pointer to a character array that represents the substring
 * we are searching for within the haystack string.
 * @param len The parameter "len" represents the length of the haystack string.
 * 
 * @return the index of the first occurrence of the needle string within the haystack string, within
 * the specified length. If the needle is not found, it returns 0.
 */
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

/**
 * The function "findUrl" takes a string as input and returns the URL found between the first and
 * second space in the string, or "NULL" if no URL is found.
 * 
 * @param req The parameter "req" is a string that represents a request.
 * 
 * @return a string. If a valid URL is found in the input string, it will return that URL. Otherwise,
 * it will return the string "NULL".
 */
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

void	*ft_memset(void *b, int c, size_t len)
{
	size_t			i;
	char			*ptr;

	ptr = (char *)b;
	i = 0;
	while (i < len)
	{
		ptr[i] = (unsigned char) c;
		i++;
	}
	return (b);
}

void	*ft_memcpy(void *dst, const void *src, size_t n)
{
	const char		*ptr_src;
	char			*ptr_dst;
	size_t			i;

	if ((dst == src) || n == 0)
		return (dst);
	if (!dst && !src)
		return (0);
	i = 0;
	ptr_src = (const char *) src;
	ptr_dst = (char *) dst;
	while (i < n)
	{
		ptr_dst[i] = ptr_src[i];
		i++;
	}
	return (dst);
}