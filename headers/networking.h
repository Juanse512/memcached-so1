#ifndef __NETWORKING_IO_H_
#define __NETWORKING_IO_H_
#include <stdint.h>

typedef struct SocketDataS {
    int fd; // file descriptor del socket
    int bin; // flag para saber si es binario
	char * buf; // buffer de entrada
	int index; // indice en el buffer
	int size; // bytes usados del buffer
	int a_len; // tamaño total del buffer
} SocketData;

int READ(int fd, char * buf, int n);

void epoll_ctl_add(int epfd, int fd, uint32_t events);

int make_socket_non_blocking (int sfd);

int read_byte(int fd, char* byte);

void input_handler_bin(int csock, int mode, char* key, char* val, int keyLen, int valLen);

int text_consume(int fd, char ** buf_p, int * index, int * size, int * a_size);

int text_consume_bin(int fd, char ** buf_p, int * index, int * size, int * a_size);

int parse_text_bin(int fd, char * buf, int buf_size, int index);

void input_handler(int csock, char ** tok);

int handle_conn(SocketData * event);

int handle_conn_bin(SocketData * event);

void * thread_f(void * arg);

void wait_for_clients();

int mk_lsock(int port);

#endif