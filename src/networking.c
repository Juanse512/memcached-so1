#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <math.h>

#include "../headers/helpers.h"
#include "../headers/hashing.h"
#include "../headers/io.h"

enum code {
	PUT = 11,
	DEL = 12,
	GET = 13,

	STATS = 21,

	OK = 101,
	EINVALID = 111,
	ENOTFOUND = 112,
	EBINARY = 113,
	EBIG = 114,
	EUNK = 115,
};

int epfd = 0;

typedef struct SocketDataS {
    int fd;
    int bin;
	char * buf;
	int index;
	int size;
	int a_len;
    // Add any other fields you might need
} SocketData;


int READ(int fd, char * buf, int n) {						\
	int rc = read(fd, buf, n);					\
	if (rc < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))	\
		return 0;						\
	if (rc <= 0)							\
		return -1;						\
	return rc; 
	}




static void epoll_ctl_add(int epfd, int fd, uint32_t events)
{
	struct epoll_event ev;
	ev.events = events;
	// ev.data.fd = fd;
	ev.data.ptr = malloc(sizeof(SocketData));
	((SocketData*)ev.data.ptr)->fd = fd;
	((SocketData*)ev.data.ptr)->buf = NULL;
	((SocketData*)ev.data.ptr)->index = 0; 
	((SocketData*)ev.data.ptr)->size = 2049; 
  /* registro fd a la instancia epoll con sus eventos asociados*/
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
		perror("epoll_ctl()\n");
		exit(1);
	}
}


static int make_socket_non_blocking (int sfd)
{
  int flags, s;
  
  /* guardo las flags asociadas al sfl */
  flags = fcntl (sfd, F_GETFL, 0);
  if (flags == -1)
  {
    perror ("fcntl");
    return -1;
  }
  
  /* agrego la flag O_NONBLOCK */
  flags |= O_NONBLOCK;
  s = fcntl (sfd, F_SETFL, flags);
  if (s == -1)
  {
    perror ("fcntl");
    return -1;
  }

  return 0;
}

int U = 0;



/*
 * Para probar, usar netcat. Ej:
 *
 *      $ nc localhost 4040
 *      NUEVO
 *      0
 *      NUEVO
 *      1
 *      CHAU
 */

// void quit(){
//     exit(0);
// }

int read_byte(int fd, char* byte){
	return READ(fd, byte, 1);
}
void input_handler_bin(int csock, int mode, char* key, char* val, int keyLen, int valLen){
	char reply[MAX_RESPONSE];
	int ok = 0;
	char comm;
	

	if(mode == 11){
		// guardar en buf
		int res = hash_word(key, val, tableSize, keyLen, valLen, 1);
		comm = OK;
		write(csock, &comm, 1);
		// sprintf(reply, "OK\n");
		pthread_mutex_lock(&putsLock);
		PUTS++;
		pthread_mutex_unlock(&putsLock);
		pthread_mutex_lock(&kvLock);
		KEYVALUES += res;
		pthread_mutex_unlock(&kvLock);
		ok = 1;
	}
	if(mode == 12){
		int res = find_elem_to_delete(key, keyLen);
		if(res){
			comm = OK;
			write(csock, &comm, 1);
		}else{
			comm = ENOTFOUND;
			write(csock, &comm, 1);
		}
		pthread_mutex_lock(&delsLock);
		DELS++;
		pthread_mutex_unlock(&delsLock);
		pthread_mutex_lock(&kvLock);
		KEYVALUES -= res;
		pthread_mutex_unlock(&kvLock);
		ok = 1;
	}

	if(mode == 13){
		Word * result = find_word(key, keyLen);
		if(result == NULL){
			// sprintf(reply, "ENOTFOUND\n");
			// sprintf(reply, "%d", 112);
			comm = ENOTFOUND;
			write(csock, &comm, 1);
		}else{
			comm = OK;
			int len = result->value.len;
			int len_net = htonl(len);
			write(csock, &comm, 1);
			write(csock, &len_net, 4);
			write(csock, result->value.string, len);
			// if(strlen(result->value) + 4 > 2048){
			// 	sprintf(reply, "EBIG\n");
			// }

			// sprintf(reply, "OK %s\n", result->value);
		}
		pthread_mutex_lock(&getsLock);
		GETS++;
		pthread_mutex_unlock(&getsLock);
		ok = 1;
	}
	if(mode == 21){

		pthread_mutex_lock(&putsLock);
		int puts = PUTS;
		pthread_mutex_unlock(&putsLock);
		pthread_mutex_lock(&getsLock);
		int gets = GETS;
		pthread_mutex_unlock(&getsLock);
		pthread_mutex_lock(&kvLock);
		int kv = KEYVALUES;
		pthread_mutex_unlock(&kvLock);
		sprintf(reply, "PUTS:%d GETS: %d KEYVALUES: %d", puts, gets, kv);
		comm = OK;
		int len = strlen(reply);
		// reply[]
		int len_net = htonl(len);
		write(csock, &comm, 1);
		write(csock, &len_net, 4);
		write(csock, reply, len);
		ok = 1;
	}
	
	if(ok == 0){
		comm = EINVAL;
		write(csock, &comm, 1);
	}
	if(mode != PUT){
		if(key){
			free(key);
		}
	}

	// sprintf(reply, "%d\n", U);
	// write(csock, reply, strlen(reply));
	return;
}
int text_consume(int fd, char ** buf_p, int * index, int * size, int * a_size){
	char * buf;
	int valid = -1;
	if(*buf_p == NULL || size == 0){
		*a_size = 2049;
		buf = malloc(sizeof(char) * (*a_size));
	}else{
		buf = *buf_p;
	}
	int rc = 1;
	int i = *size;
	while ((rc = read(fd, buf + i, 1)) > 0) {
		if (buf[i] == '\n'){
			valid = 1;
			break;

		}
		i++;
		if(i >= *a_size)
			return -2;
		
	}
	if(i == *size){
		return 0;
	}
	buf[i] = '\0';
	*size = i;
	*buf_p = buf;
	
	return valid;
}
int text_consume_bin(int fd, char ** buf_p, int * index, int * size, int * a_size){
	char * buf;
	if(*buf_p == NULL || size == 0){
		*a_size = 2049;
		buf = malloc(sizeof(char) * (*a_size));
		while(buf == NULL){
			freeMemory();
			printf("FREE MEMORY TEXT CONSUME BIN\n");
			buf = malloc(sizeof(char) * (*a_size));
		}
	}else{
		buf = *buf_p;
	}
	int rc; // = 1;
	int i = *size;
	while ((rc = read(fd, buf + i, 1)) > 0) {
		// rc = read(fd, buf + i, 1);
		i++;
		
		if(i >= *a_size){
			*a_size = (*a_size) * 2;
			*buf_p = realloc(*buf_p, (*a_size));
			buf = *buf_p;
			// eliminar si no hay memoria?
		}
	}
	if(i == *size){
		return 0;
	}
	*size = i;
	*buf_p = buf;
	return 1;
}

int parse_text_bin(int fd, char * buf, int buf_size, int index){
	// MODO STATS
	if(index >= buf_size){
		return 0;
	}
	if(buf_size == 1 && buf[0] == STATS){
		input_handler_bin(fd, STATS, NULL, NULL, 0, 0);
		return parse_text_bin(fd, buf, buf_size, index + 1);
	}
	if(buf_size < 5){
		return -1;
	}
	
	int mode = buf[index];
	char * key = NULL;
	char * value = NULL;

	int size = ((int)buf[index+1] * pow(256,3)) + ((int)buf[index+2] * pow(256,2)) + ((int)buf[index+3] * pow(256,1)) + ((int)buf[index+4]);
	int vSize = 0;
	if((buf_size-5-index) < size){

		return -1;
	}

	if(mode == PUT && (size + 9) > buf_size){
			return -1;
	}

	key = malloc(sizeof(char)*(size+1));
	while(key == NULL){
		freeMemory();
		printf("FREE MEMORY PARSE TEXT BIN\n");
		key = malloc(sizeof(char)*(size+1));
	}
	int k = 0;
	// liberar memoria de nuevo?

	for(int i = (index + 5); i < (size+index+5); i++){
		key[k++] = buf[i];
	}
	key[k] = '\0';
	printf("key: %s mode: %d\n", key, buf[0]);
	index = (size + index + 5);
	if(mode == PUT){
		vSize = ((int)buf[index] * pow(256,3)) + ((int)buf[index+1] * pow(256,2)) + ((int)buf[index+2] * pow(256,1)) + ((int)buf[index+3]);
		if((buf_size-4-index) < vSize){

			return -1;
		}

		value = malloc(sizeof(char)*(vSize+1));
		while(value == NULL){
			printf("FREE MEMORY PARSE TEXT BIN VALUE\n");
			freeMemory();
			value = malloc(sizeof(char)*(vSize+1));
		}
		k = 0;
		for(int i = (index + 4); i < (vSize+index+4); i++){
			value[k++] = buf[i];
		}
		index = (vSize + index + 4);
	}
	// llamar a handle 
	input_handler_bin(fd, mode, key, value, size, vSize);
	return parse_text_bin(fd, buf, buf_size, index);
}


void input_handler(int csock, char ** tok){
	char reply[MAX_RESPONSE];
	int ok = 0;
	if(strcmp(tok[0], "GET") == 0){
		Word * result = find_word(tok[1], strlen(tok[1]));
		if(result == NULL){
			sprintf(reply, "ENOTFOUND\n");
		}else{
			if(result->bin){
				sprintf(reply, "EBIN\n");
			}else{
				if(result->value.len + 4 > 2048){
					sprintf(reply, "EBIG\n");
				}else{
					sprintf(reply, "OK %s\n", result->value.string);
				}
			}
		}
		pthread_mutex_lock(&getsLock);
		GETS++;
		pthread_mutex_unlock(&getsLock);
		ok = 1;
	}
	if(strcmp(tok[0], "PUT") == 0){
		int len = strlen(tok[1]);
		int lenV = strlen(tok[2]);
		char * key = malloc(sizeof(char) * len);
		char * value = malloc(sizeof(char) * lenV);
		memcpy(key, tok[1], len);
		memcpy(value, tok[2], lenV);
		int res = hash_word(key, value, tableSize, strlen(tok[1]), strlen(tok[2]), 0);
		sprintf(reply, "OK\n");
		pthread_mutex_lock(&putsLock);
		PUTS++;
		pthread_mutex_unlock(&putsLock);
		pthread_mutex_lock(&kvLock);
		KEYVALUES += res;
		pthread_mutex_unlock(&kvLock);
		ok = 1;
	}

	if(strcmp(tok[0], "DEL") == 0){
		int res = find_elem_to_delete(tok[1], strlen(tok[1]));
		if(res){
			sprintf(reply, "OK\n");
		}else{
			sprintf(reply, "ENOTFOUND\n");
		}
		pthread_mutex_lock(&delsLock);
		DELS++;
		pthread_mutex_unlock(&delsLock);
		pthread_mutex_lock(&kvLock);
		KEYVALUES -= res;
		pthread_mutex_unlock(&kvLock);
		ok = 1;
	}

	if(strcmp(tok[0], "STATS") == 0){
		// deadlock? 
		pthread_mutex_lock(&putsLock);
		int puts = PUTS;
		pthread_mutex_unlock(&putsLock);
		pthread_mutex_lock(&getsLock);
		int gets = GETS;
		pthread_mutex_unlock(&getsLock);
		pthread_mutex_lock(&kvLock);
		int kv = KEYVALUES;
		pthread_mutex_unlock(&kvLock);
		sprintf(reply, "OK PUTS:%d GETS: %d KEYVALUES: %d\n", puts, gets, kv);

		ok = 1;
	}
	if(ok == 0){
		sprintf(reply, "EINVAL\n");
	}
	// sprintf(reply, "%d\n", U);
	write(csock, reply, strlen(reply));
	return;
}



int handle_conn(SocketData * event)
{
	char tok[3][1000];
	int res_text;
	while (1) {
		/* Atendemos pedidos, uno por linea */
		// rc = fd_readline(csock, buf);
		res_text = text_consume(event->fd, &(event->buf), &(event->index), &(event->size), &(event->a_len));
		if(res_text == 0){
			close(event->fd);
			return 1;
		}
		if(res_text == -1){
			return 2;
		}
		if(res_text == -2){
			// EBIG
			return 2;
		}
		char ** tokP = parser(event->buf);
		// parser(event->buf, tok);
		input_handler(event->fd, tokP);
		if(res_text == 1){
			if(event->buf)
				free(event->buf);
			
			event->buf = NULL;
			event->a_len = 0;
			event->index = 0;
			event->size = 0;
		}

        return 2;

	}
}

int handle_conn_bin(SocketData * event)
{
	// char buf[MAX_RESPONSE];
	int res, res_text;
	while (1) {


		res_text = text_consume_bin(event->fd, &(event->buf), &(event->index), &(event->size), &(event->a_len));
		if(res_text == 0){
			close(event->fd);
			return 1;
		}
		res = parse_text_bin(event->fd, (event->buf), event->size, event->index);
		if(res == 0){
			if(event->buf)
				free(event->buf);
			
			event->buf = NULL;
			event->a_len = 0;
			event->index = 0;
			event->size = 0;
		}
		return 2;
	}
}



void * thread_f(void * arg){
    // int lsock = arg - (void*)0;
    int nfds; //lleva la cantidad de fds listos para E/S
	int efd, s;
    struct epoll_event events[10]; // ver max events
	struct epoll_event event;
    while(1){
        nfds = epoll_wait(epfd, events, 10, -1);  //chequear error
        for (int i = 0; i < nfds; i++) {
			SocketData *eventData = ((SocketData *)events[i].data.ptr);
			if (lsock == (eventData->fd))
			{ 
                
                    //struct sockaddr in_addr;
                        //socklen_t in_len;
                        int infd;
                        //in_len = sizeof in_addr;
                        infd = accept (lsock, NULL, NULL);
                        s = make_socket_non_blocking (infd);
                        if (s == -1)
                        abort ();

						// event.data.u32 = (uint32_t)0;
                        event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                        // event.data.fd = infd;
						
						SocketData* ctx = malloc(sizeof(SocketData));
						ctx->fd = infd;
						ctx->bin = 0;
						ctx->buf = NULL;
						ctx->index = 0; 
						ctx->size = 0; 
						ctx->a_len = 2049; 
						event.data.ptr = ctx;
                        s = epoll_ctl (epfd, EPOLL_CTL_ADD, infd, &event);
                        if (s == -1)
                        {
                            perror ("epoll_ctl");
                            abort ();
                        }
						event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
						event.data.ptr = eventData;
						epoll_ctl(efd, EPOLL_CTL_MOD, eventData->fd, &event);
        /*              continue; */
                }else{
						if (binlsock == (eventData->fd)){
							int infd;
							//in_len = sizeof in_addr;

							infd = accept (binlsock, NULL, NULL);
							s = make_socket_non_blocking (infd);
							if (s == -1)
								abort ();

							// event.data.u32 = (uint32_t)1;
							event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
							// event.data.fd = infd;
							SocketData* ctx = malloc(sizeof(SocketData));
							ctx->fd = infd;
							ctx->bin = 1;
							ctx->buf = NULL;
							ctx->index = 0; 
							ctx->size = 0; 
							ctx->a_len = 2049; 
							event.data.ptr = ctx;
							s = epoll_ctl (epfd, EPOLL_CTL_ADD, infd, &event);
							// epoll_ctl(epfd, EPOLL_CTL_MOD, binlsock, &event);
							if (s == -1)
							{
								perror ("epoll_ctl");
								abort ();
							}
							event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
							event.data.ptr = eventData;
							epoll_ctl(efd, EPOLL_CTL_MOD, eventData->fd, &event);
						}else{
                            int done = 0;

							// int type = *((int*)events[i].data.ptr);
							if((eventData->bin) == 0){
                            	done = handle_conn(eventData);
							}else{
								done = handle_conn_bin(eventData);
								// done = 2;
							}


                            if (done == 1)
                            {
                                printf ("Closed connection on descriptor %d\n",
                                        (((SocketData*)events[i].data.ptr)->fd));
                                close (((SocketData*)events[i].data.ptr)->fd);
								if(events[i].data.ptr){
									free(events[i].data.ptr);
									events[i].data.ptr = NULL;
								}
                            }else{
								event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
								event.data.ptr = eventData;
								epoll_ctl(epfd, EPOLL_CTL_MOD, eventData->fd, &event);
							}
						}
                }
            }
    }
    return NULL;
}


void wait_for_clients()
{
	// int csock;
    int number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
    pthread_t clientes[number_of_processors+1];
    for (int i = 0; i < number_of_processors; i++){
		pthread_create(&clientes[i], NULL, thread_f, NULL);
    }
    for (int i = 0; i < number_of_processors; i++){
        pthread_join(clientes[i], NULL);
		// pthread_create(&clientes[i], NULL, thread_f, lsock + (void*)0);
    }
	/* Esperamos una conexión, no nos interesa de donde viene */
    return;
}

/* Crea un socket de escucha en puerto 4040 TCP */
int mk_lsock(int port)
{
	struct sockaddr_in sa;
	int sock;
	int rc;
	int yes = 1;

	/* Crear socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		quit("socket");

	/* Setear opción reuseaddr... normalmente no es necesario */
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == 1)
		quit("setsockopt");

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	/* Bindear al puerto 4040 TCP, en todas las direcciones disponibles */
	rc = bind(sock, (struct sockaddr *)&sa, sizeof sa);
	if (rc < 0)
		quit("bind");

	/* Setear en modo escucha */
	rc = listen(sock, 10);
  
  /* creo instancia epoll y registro el sock */
  if(epfd == 0){
  	epfd = epoll_create(1); 
  }
  epoll_ctl_add(epfd, sock, EPOLLIN | EPOLLOUT | EPOLLET);
	
  if (rc < 0)
		quit("listen");

	return sock;
}

// int main()
// {
// 	int lsock;
// 	lsock = mk_lsock();
// 	wait_for_clients(lsock);
// }
