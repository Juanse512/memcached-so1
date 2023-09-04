#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include "../headers/helpers.h"
#include "../headers/io.h"
void quit(const char *s)
{
	perror(s);
	exit(1);
}
//readn: (int, void *, size_t) -> (ssize_t)
// Read confiable en caso de que en el primer read no se lea toda la cantidad requerida
ssize_t readn(int fd, void *buffer, size_t n){
	ssize_t numRead; 
	size_t totRead;	 
	char *buf;
	buf = buffer; 
	for (totRead = 0; totRead < n;)
	{
		numRead = read(fd, buf, n - totRead);
		if (numRead == 0)	
			return totRead; 
		if (numRead == -1)
		{
			if (errno == EINTR)
				continue; 
			else
				return -1; 
		}
		totRead += numRead;
		buf += numRead;
	}
	return totRead; /* Must be 'n' bytes if we get here */
}
//writen: (int, void *, size_t) -> (ssize_t)
// Write confiable en caso de que en el primer write no se escriba toda la cantidad requerida
ssize_t writen(int fd, const void *buffer, size_t n){
	ssize_t numWritten; 
	size_t totWritten;	
	const char *buf;
	buf = buffer; 

	for (totWritten = 0; totWritten < n;)
	{
		numWritten = write(fd, buf, n - totWritten);
		if (numWritten <= 0)
		{
			if (numWritten == -1 && errno == EINTR)
				continue; 
			else
				return -1;
			
		}
		totWritten += numWritten;
		buf += numWritten;
	}
	return totWritten;
}