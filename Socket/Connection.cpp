#include "Connection.hpp"

std::string Connection::getHeader(void)
{
    return (this->header);
}
char    *Connection::getBody(void)
{
    return (this->body);
}
unsigned long Connection::getBodySize(void)
{
    return (this->bodySize);
}

unsigned long getBodyLenght(std::string header)
{
	int	pos;
	unsigned long	newLinePos;
	unsigned long	len;

	len = 0;
	pos = header.find("Content-Length");
	if ((unsigned long)pos != std::string::npos)
	{
		newLinePos = header.find("\r", pos);
		if (newLinePos != std::string::npos)
			len = atoi(header.substr((pos + 15), newLinePos - pos).c_str());
	}
	return (len);
}

void    Connection::addToBuffer(const char *toAdd)
{
    char            *newBuffer;
    unsigned long   i, j;

    newBuffer = new char [this->oldBufferLen + this->newBufferLen];
    memset(newBuffer, 0, this->oldBufferLen + this->newBufferLen);
    for (i = 0; i < this->oldBufferLen; i++)
        newBuffer[i] = this->buffer[i];
    for (j = 0; j < this->newBufferLen; j++)
        newBuffer[i + j] = toAdd[j];
    if (this->buffer)
        delete [] this->buffer; 
    this->buffer = newBuffer;
}

int     Connection::handleBody(int &maxBodySizeExeeded, int maxBodySize)
{
    unsigned long   byteAlreadyRead = this->oldBufferLen + this->newBufferLen;

    if (this->headerSize > 0)
    {
        this->bodySize = getBodyLenght(this->header);
        if (this->bodySize > (unsigned long)maxBodySize)
        {
            maxBodySizeExeeded = 1;
            return (1);
        }
        maxBodySizeExeeded = 0;
        if (this->bodySize > 0 && (this->headerSize + this->bodySize) <= byteAlreadyRead)
        {
            this->body = new char [this->bodySize];
            memcpy(this->body, ft_strnstr(this->buffer, "\r\n\r\n", this->oldBufferLen + this->newBufferLen) + 4, this->bodySize);
            return (1);
        }
        if (this->bodySize == 0 || (this->headerSize + this->bodySize) <= byteAlreadyRead)
            return (1);
    }
    return 0;
}

void    Connection::read(int &maxBodySizeExeeded, int maxBodySize)
{
    static unsigned long    toRead = 3000;
    char                    *tempBuffer;
    int                     recvRet = 0;

    if (this->handleBody(maxBodySizeExeeded, maxBodySize))
    {
        this->pollfd->events = POLLOUT;
        	std::cout << this->buffer << std::endl;
        return ;
    }
    tempBuffer = new char [toRead];
    memset(tempBuffer, 0, toRead);
    recvRet = recv(this->fd, tempBuffer, toRead, MSG_DONTWAIT);
    this->oldBufferLen += this->newBufferLen;
    this->newBufferLen = recvRet;
    this->addToBuffer(tempBuffer);
    delete [] tempBuffer;
    if (ft_strnstr(this->buffer, "\r\n\r\n", this->oldBufferLen + this->newBufferLen))
    {
        this->headerSize = headerLen(this->buffer, "\r\n\r\n", this->oldBufferLen + this->newBufferLen);
        this->header = std::string(this->buffer, this->headerSize + 2);
        if (this->handleBody(maxBodySizeExeeded, maxBodySize))
        {
            this->pollfd->events = POLLOUT;
            return ;
        }
    }
}

Connection &Connection::operator=(const Connection &cpy)
{
    this->buffer = cpy.buffer;
    this->newBufferLen = cpy.newBufferLen;
    this->oldBufferLen = cpy.oldBufferLen;
    this->pollfd = cpy.pollfd;
    this->body = cpy.body;
    this->headerSize = cpy.headerSize;
    this->bodySize = cpy.bodySize;
    this->header = cpy.header;
    this->fd = cpy.fd;
    return (*this);
}

Connection::Connection(const Connection &cpy)
{
    *this = cpy;
}

Connection::Connection(struct pollfd *pollfd)
{
    this->newBufferLen = 0;
    this->oldBufferLen = 0;
    this->headerSize = 0;
    this->bodySize = 0;
    this->buffer = 0;
    this->body = 0;
    this->pollfd = pollfd;
    this->fd = pollfd->fd;
}

Connection::~Connection()
{
    if (this->buffer)
        delete [] this->buffer;
    if (this->body)
        delete [] this->body;
}