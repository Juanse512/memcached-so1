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
#include "../headers/helpers.h"
#include "../headers/hashing.h"
int epfd = 0;
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
	while ((rc = read(fd, buf + i, 1)) > 0) {
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



static void epoll_ctl_add(int epfd, int fd, uint32_t events)
{
	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;
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
		pthread_mutex_lock(&getsLock);
		pthread_mutex_lock(&kvLock);
			sprintf(reply, "OK PUTS:%d GETS: %d KEYVALUES: %d\n", PUTS, GETS, KEYVALUES);
		pthread_mutex_unlock(&putsLock);
		pthread_mutex_unlock(&getsLock);
		pthread_mutex_unlock(&kvLock);
		ok = 1;
	}
	if(ok == 0){
		sprintf(reply, "EINVAL\n");
	}
	// sprintf(reply, "%d\n", U);
	write(csock, reply, strlen(reply));
	return;
}

int handle_conn(int csock)
{
	char buf[MAX_RESPONSE];
	char tok[3][1000];
	int rc;

	while (1) {
		/* Atendemos pedidos, uno por linea */
		rc = fd_readline(csock, buf);
		if(rc == -1){
			char reply[20];
			sprintf(reply, "EINVAL\n");
			write(csock, reply, strlen(reply));
			return 2;
		}
		if (rc == 0) {
			/* linea vacia, se cerró la conexión */
			close(csock);
			return 1;
		}
        // printf("%ld\n", pthread_self());
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
            if (lsock == events[i].data.fd)
            { 
                
                    //struct sockaddr in_addr;
                        //socklen_t in_len;
                        int infd;
                        //in_len = sizeof in_addr;
                        infd = accept (lsock, NULL, NULL);
                        s = make_socket_non_blocking (infd);
                        if (s == -1)
                        abort ();

						// event.data.ptr = &text;
                        event.data.fd = infd;
                        event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                        printf("set events %u, infd=%d\n", event.events, infd);
                        s = epoll_ctl (epfd, EPOLL_CTL_ADD, infd, &event);
                        if (s == -1)
                        {
                            perror ("epoll_ctl");
                            abort ();
                        }
        /*              continue; */
                }else{
						if (binlsock == events[i].data.fd){
							printf("BINARY MODE\n");
							int infd;
							//in_len = sizeof in_addr;
							infd = accept (binlsock, NULL, NULL);
							s = make_socket_non_blocking (infd);
							if (s == -1)
							abort ();

							// event.data.ptr = &bin;
							event.data.fd = infd;
							event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
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
							// printf("type: %d\n", *((int*)events[i].data.ptr));
							// int type = *((int*)events[i].data.ptr);
							// if(type == 0){
                            	done = handle_conn(events[i].data.fd);
							// }else{
							// 	done = 2;
							// 	printf("BINARY MODE\n");
							// }


                            if (done == 1)
                            {
                                printf ("Closed connection on descriptor %d\n",
                                        events[i].data.fd);
                                close (events[i].data.fd);

                            }else{
								event.events = EPOLLIN | EPOLLONESHOT;
								event.data.fd = events[i].data.fd;
								epoll_ctl(epfd, EPOLL_CTL_MOD, events[i].data.fd, &event);
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
