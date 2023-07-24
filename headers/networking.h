#ifndef __NETWORKING_IO_H_
#define __NETWORKING_IO_H_
#include <stdint.h>


static void epoll_ctl_add(int epfd, int fd, uint32_t events);

static int make_socket_non_blocking (int sfd);

void * thread_f(void * arg);

void wait_for_clients(int lsock);

int mk_lsock(int port);

#endif