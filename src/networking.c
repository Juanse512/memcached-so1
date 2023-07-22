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
    // Add any other fields you might need
} SocketData;


#define READ(fd, buf, n) ({						\
	int rc = read(fd, buf, n);					\
	if (rc < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))	\
		return 0;						\
	if (rc <= 0)							\
		return -1;						\
	rc; })



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

int fd_readline(int fd, char *buf)
{
	int rc;
	int i = 0;

	/*
	 * Leemos de a un caracter (no muy eficiente...) hasta
	 * completar una línea.
	 */
	while ((rc = READ(fd, buf + i, 1)) > 0) {
		if(i > 2048){
			i = -1;
			break;
		}
		
		if (buf[i] == '\n')
			break;
		i++;
	}

	if (rc < 0)
		return rc;

	buf[i] = 0;
	return i;
}

int read_byte(int fd, char* byte){
	return READ(fd, byte, 1);
}

char * fd_readline_bin(int fd, char *mode){
	int rc;
	int i = 0;
	int size = 0;
	char b1,b2,b3,b4;
	int r1,r2,r3,r4;
	char * buf;
	if(mode != NULL){
		rc = READ(fd, mode, 1);
		if(rc <= 0){
			return 0;
		}
	}
	r1 = read_byte(fd, &b1);
	r2 = read_byte(fd, &b2);
	r3 = read_byte(fd, &b3);
	r4 = read_byte(fd, &b4);
	printf("BEF RETURN\n");
	// printf("read bytes: %d %d %d %d %d\n", r1,r2,r3,r4);
	if(r1 && r2 && r3 && r4){
		size = ((int)b1 * pow(256,3)) + ((int)b2 * pow(256,2)) + ((int)b3 * pow(256,1)) + ((int)b4);

		printf("size: %d %d %d %d %d\n", size, (int)b1, (int)b2, (int)b3, (int)b4);
		
		buf = malloc(sizeof(char)*(size+2));
		while (i < size) {
			rc = READ(fd, buf + i, size - i);
			if (rc <= 0)
				break;
			i += rc;
		}
		buf[size] = '\0';
		// printf("AFTER READ\n");
	}else{
		printf("FAILED TO READ SIZE\n");
		// while(1){
		// 	rc = read_byte(fd, &b1);
		// 	if(rc){
		// 		printf("%d\n", b1);
		// 	}
		// };
		// printf("byte %d\n", b1);
		return NULL;
	}
	if(rc <= 0){
		printf("FAILED TO READ DATA %d\n", rc);
		return NULL;
	}
	return buf;
}


static void epoll_ctl_add(int epfd, int fd, uint32_t events)
{
	struct epoll_event ev;
	ev.events = events;
	// ev.data.fd = fd;
	ev.data.ptr = malloc(sizeof(SocketData));
	((SocketData*)ev.data.ptr)->fd = fd;
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

void input_handler(int csock, char tok[3][1000]){
	char reply[MAX_RESPONSE];
	int ok = 0;
	if(strcmp(tok[0], "GET") == 0){
		Word * result = find_word(tok[1]);
		if(result == NULL){
			sprintf(reply, "ENOTFOUND\n");
		}else{
			if(strlen(result->value) + 4 > 2048){
				sprintf(reply, "EBIG\n");
			}
			sprintf(reply, "OK %s\n", result->value);
		}
		pthread_mutex_lock(&getsLock);
		GETS++;
		pthread_mutex_unlock(&getsLock);
		ok = 1;
	}
	if(strcmp(tok[0], "PUT") == 0){
		int res = hash_word(tok[1], tok[2], tableSize);
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
		int res = find_elem_to_delete(tok[1]);
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

void input_handler_bin(int csock, int mode, char* key, char* val){
	char reply[MAX_RESPONSE];
	int ok = 0;
	char comm;
	printf("MODE HANDLER: %d\n", mode);
	if(mode == 11){
		int res = hash_word(key, val, tableSize);
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
		int res = find_elem_to_delete(key);
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
		Word * result = find_word(key);
		if(result == NULL){
			// sprintf(reply, "ENOTFOUND\n");
			// sprintf(reply, "%d", 112);
			comm = ENOTFOUND;
			write(csock, &comm, 1);
		}else{
			printf("FOUND\n");
			comm = OK;
			int len = strlen(result->value);
			int len_net = htonl(len);
			write(csock, &comm, 1);
			write(csock, &len_net, 4);
			write(csock, result->value, len);
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
	printf("AFTER HANDLER\n");
	// sprintf(reply, "%d\n", U);
	// write(csock, reply, strlen(reply));
	return;
}

int handle_conn(int csock)
{
	char buf[MAX_RESPONSE];
	char tok[3][1000];
	int rc;

	while (1) {
		/* Atendemos pedidos, uno por linea */
		printf("rc: %d\n", rc);
		rc = fd_readline(csock, buf);

		// if(rc == -1){
		// 	char reply[20];
		// 	sprintf(reply, "EINVAL\n");
		// 	write(csock, reply, strlen(reply));
		// 	return 2;
		// }
		if (rc == 0 || rc == -1) {
			/* linea vacia, se cerró la conexión */
			close(csock);
			return 1;
		}
		parser(buf, tok);
		// printf("%s\n", tok[2]);
        input_handler(csock, tok);
        return 2;
		// if (!strcmp(buf, "NUEVO")) {
		// 	char reply[20];
		// 	sprintf(reply, "%d\n", U);
		// 	U++;
		// 	write(csock, reply, strlen(reply));
		// 	return 2;
		// } else if (!strcmp(buf, "CHAU")) {
		// 	close(csock);
		// 	return 1;
		// }
	}
}

int handle_conn_bin(int csock)
{
	// char buf[MAX_RESPONSE];
	char *buf = NULL;
	char *val= NULL;
	char tok[3][1000];
	int rc;
	char mode = 0;
	while (1) {
		/* Atendemos pedidos, uno por linea */
		printf("ENTER HANDLE CONN BIN\n");

		buf = fd_readline_bin(csock, &mode);
		printf("mode: %d %p\n", mode, buf);
		printf("AFTER READLINE BIN\n");
		if(mode == 11){
			val = fd_readline_bin(csock, NULL);
			printf("value: %s\n", val);
		}
		if ((buf == NULL && (mode != STATS)) || mode == 0) {
			/* linea vacia, se cerró la conexión */
			printf("CLOSE CSOCK\n");
			close(csock);
			return 1;
		}
		// parser(buf, tok);
        input_handler_bin(csock, mode, buf, val);
		// printf("SEG FAULT %p\n", &mode);
		if(buf != NULL){
			free(buf);
		}
        return 2;

	}
}



void * thread_f(void * arg){
    int lsock = arg - (void*)0;
    int nfds; //lleva la cantidad de fds listos para E/S
    struct epoll_event events[10]; // ver max events
	struct epoll_event event;
	int s;
	int efd;
	int bin = 1;
	int text = 0;
    while(1){
        nfds = epoll_wait(epfd, events, 10, -1);  //chequear error
        for (int i = 0; i < nfds; i++) {
			printf("HERE\n");
            if (lsock == (((SocketData*)events[i].data.ptr)->fd))
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
						
						event.data.ptr = malloc(sizeof(SocketData));
						((SocketData*)event.data.ptr)->fd = infd;
						((SocketData*)event.data.ptr)->bin = 0;
                        printf("set events %u, infd=%d\n", event.events, infd);
                        s = epoll_ctl (epfd, EPOLL_CTL_ADD, infd, &event);
                        if (s == -1)
                        {
                            perror ("epoll_ctl");
                            abort ();
                        }
        /*              continue; */
                }else{
						if (binlsock == (((SocketData*)events[i].data.ptr)->fd)){
							int infd;
							//in_len = sizeof in_addr;
							printf("BINARY MODE\n");

							infd = accept (binlsock, NULL, NULL);
							s = make_socket_non_blocking (infd);
							if (s == -1)
								abort ();

							// event.data.u32 = (uint32_t)1;
							event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
							// event.data.fd = infd;
							event.data.ptr = malloc(sizeof(SocketData));
							((SocketData*)event.data.ptr)->fd = infd;
							((SocketData*)event.data.ptr)->bin = 1;
							printf("set events %u, infd=%d\n", event.events, infd);
							s = epoll_ctl (epfd, EPOLL_CTL_ADD, infd, &event);
							if (s == -1)
							{
								perror ("epoll_ctl");
								abort ();
							}
						}else{
                            int done = 0;

                            ssize_t count;
                            char buf[512];
							printf("type: %d\n", (((SocketData*)events[i].data.ptr)->bin));
							// int type = *((int*)events[i].data.ptr);
							if((((SocketData*)events[i].data.ptr)->bin) == 0){
                            	done = handle_conn(((SocketData*)events[i].data.ptr)->fd);
							}else{
								printf("BINARY MODE HANDLE\n");
								done = handle_conn_bin(((SocketData*)events[i].data.ptr)->fd);
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
								event.events = EPOLLIN | EPOLLONESHOT;
								((SocketData*)event.data.ptr)->fd = (((SocketData*)events[i].data.ptr)->fd);
								epoll_ctl(epfd, EPOLL_CTL_MOD, (((SocketData*)events[i].data.ptr)->fd), &event);
							}
						}
                }
            }
    }
    return NULL;
}


void wait_for_clients(int lsock)
{
	// int csock;
	int nfds; //lleva la cantidad de fds listos para E/S
	struct epoll_event events[10];
	struct epoll_event event;
	int s;
	int efd;
    int number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
    pthread_t clientes[number_of_processors+1];
    for (int i = 0; i < number_of_processors; i++){
		pthread_create(&clientes[i], NULL, thread_f, lsock + (void*)0);
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
	int lsock;
	int rc;
	int yes = 1;

	/* Crear socket */
	lsock = socket(AF_INET, SOCK_STREAM, 0);
	if (lsock < 0)
		quit("socket");

	/* Setear opción reuseaddr... normalmente no es necesario */
	if (setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == 1)
		quit("setsockopt");

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	/* Bindear al puerto 4040 TCP, en todas las direcciones disponibles */
	rc = bind(lsock, (struct sockaddr *)&sa, sizeof sa);
	if (rc < 0)
		quit("bind");

	/* Setear en modo escucha */
	rc = listen(lsock, 10);
  
  /* creo instancia epoll y registro el lsock */
  if(epfd == 0){
  	epfd = epoll_create(1); 
  }
  epoll_ctl_add(epfd, lsock, EPOLLIN | EPOLLOUT | EPOLLET);
	
  if (rc < 0)
		quit("listen");

	return lsock;
}

// int main()
// {
// 	int lsock;
// 	lsock = mk_lsock();
// 	wait_for_clients(lsock);
// }
