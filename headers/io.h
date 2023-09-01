#ifndef __SPELL_IO_H_
#define __SPELL_IO_H_
#include "helpers.h"

void quit(const char *s);

ssize_t readn(int fd, void *buffer, size_t n);

ssize_t writen(int fd, const void *buffer, size_t n);

#endif