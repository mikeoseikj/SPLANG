#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>


#include "lib.h"

// Note:  Library functions shouldn't be prefixed with an underscore because of name mangling of user defined function by the compiler

int openfile(char *filename, char *mode)
{
	int fd;
	if(strcmp(mode, "r") == 0)
	{
		fd = open(filename, O_RDONLY);
	}
	else if(strcmp(mode, "w") == 0)
	{
		fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC);
	}
	else if(strcmp(mode, "a") == 0)
	{
		fd = open(filename, O_WRONLY|O_CREAT|O_APPEND);
	}
	else if(strcmp(mode, "r+") == 0)
	{
		fd = open(filename, O_RDWR);
	}
	else if(strcmp(mode, "w+") == 0)
	{
		fd = open(filename, O_RDWR|O_CREAT|O_TRUNC);
	}
	else if(strcmp(mode, "a+") == 0)
	{
		fd = open(filename, O_RDWR|O_CREAT|O_APPEND);
	}
	else
	{
		errno = ENOTSUP;	// operation not supported
		fd = -1;
	}
	return fd;
}

int readfile(int fd, char *buf, int count)
{
	int size = read(fd, buf, count);
	return size;
}

int writefile(int fd, char *buf, int count)
{
	int size = write(fd, buf, count);
	return size;
}

int closefile(int fd)
{
	int ret = close(fd);
	return ret;
}

void sys_error(char *s)
{
	perror(s);
}

char *alloca(int size)
{
	struct alloca_symbol *sym = malloc(sizeof(struct alloca_symbol));
	char *addr = malloc(size);
	sym->address = addr;
	alloca_enqueue(&alloca_symbol_list, sym);
	return addr;
}

void cleanup_alloca()
{
	struct alloca_symbol *sym = alloca_symbol_list;
	while(sym)
	{
		free(sym->address);
		struct alloca_symbol *temp = sym;
		sym = sym->next;
		free(temp);
	}
	alloca_symbol_list = NULL;
}

/* Math funcitons*/
float pow(float x, float y)
{
	return (powf(x, y));
}

float sqrt(float x)
{
	return (sqrtf(x));
}
	
float sin(float x)
{
	return (sinf(x));
}

float cos(float x)
{
	return (cosf(x));
}

float tan(float x)
{
	return (tanf(x));
}

float log(float x)
{
	return (logf(x));
}

float log2(float x)
{
	return (log2f(x));
}

float log10(float x)
{
	return (log10f(x));
}

