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
#include "../headers/networking.h"
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
	EMEM = 116
};

int epfd = 0;


int READ(int fd, char * buf, int n) {						\
	int rc = read(fd, buf, n);					\
	if (rc < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))	\
		return 0;						\
	if (rc <= 0)							\
		return -1;						\
	return rc; 
	}


//epoll_ctl_add: (int, int, uint32_t) -> ()
// Agrega un socket a la instancia de epoll
void epoll_ctl_add(int epfd, int fd, uint32_t events)
{
	struct epoll_event ev;
	ev.events = events;
	ev.data.ptr = robust_malloc(sizeof(SocketData), 1);
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

//make_socket_non_blocking: (int) -> (int)
// Toma un file descriptor y hace el socket no bloqueante
int make_socket_non_blocking (int sfd)
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

//read_byte(int, char*) -> (int)
// Lee un byte de un file descriptor y lo guarda en el char* byte
int read_byte(int fd, char* byte){
	return READ(fd, byte, 1);
}
//inpunt_handler_bin: (int, int, char*, char*, int, int) -> ()
// Handler de input del modo binario, dado los argumentos necesarios para hacer una request, llama a las funciones necesarias
// para ejecutarla, ademas actualiza los valores del STATS
void input_handler_bin(int csock, int mode, char* key, char* val, int keyLen, int valLen){
	char reply[MAX_RESPONSE];
	int ok = 0;
	char comm;
	

	if(mode == PUT){
		int res = hash_word(key, val, tableSize, keyLen, valLen, 1);
		// Si devuelve -1 significa que no hay memoria disponible, devuelvo EMEM
		if(res == -1){
			comm = EMEM;
			write(csock, &comm, 1);
		}else{
			comm = OK;
			write(csock, &comm, 1);
			pthread_mutex_lock(&putsLock);
			PUTS++;
			pthread_mutex_unlock(&putsLock);
			pthread_mutex_lock(&kvLock);
			KEYVALUES += res;
			pthread_mutex_unlock(&kvLock);
		}
		ok = 1;
	}
	if(mode == DEL){
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

	if(mode == GET){
		Word * result = find_word(key, keyLen);
		if(result == NULL){
			comm = ENOTFOUND;
			write(csock, &comm, 1);
		}else{
			comm = OK;
			int len = result->value.len;
			int len_net = htonl(len);
			write(csock, &comm, 1);
			write(csock, &len_net, 4);
			write(csock, result->value.string, len);
		}
		pthread_mutex_lock(&getsLock);
		GETS++;
		pthread_mutex_unlock(&getsLock);
		ok = 1;
	}
	if(mode == STATS){

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
	// Libero la key si no la guarde usando PUT
	if(mode != PUT){
		if(key){
			free(key);
		}
	}
	return;
}
//text_consume: (int, char **, int *, int *, int *) -> (int)
// Consume el buffer del file descriptor en modo texto
// Devuelve -1 si no se termino de consumir un comando entero, -2 si el comando es demasiado grande para este modo
// -3 si no tengo memoria disponible, 0 si no hay nada que leer y 1 si se pudo leer y no hubo problemas
int text_consume(int fd, char ** buf_p, int * index, int * size, int * a_size){
	char * buf;
	int valid = -1;
	// Si no tengo buffer anterior, lo creo
	if(*buf_p == NULL || size == 0){
		*a_size = 2049;
		buf = robust_malloc(sizeof(char) * (*a_size), 0);
		if(buf == NULL)
			return -3;
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
//text_consume_bin: (int, char **, int *, int *, int *) -> (int)
// Consume el buffer del file descriptor en modo binario
// Devuelve -1 si no hay memoria disponible, 0 si no hay nada que leer y 1 si todo salio correctamente
int text_consume_bin(int fd, char ** buf_p, int * index, int * size, int * a_size){
	char * buf;
	if(*buf_p == NULL || size == 0){
		*a_size = 2049;
		buf = robust_malloc(sizeof(char) * (*a_size), 0);
		if(buf == NULL)
			return -1;
	}else{
		buf = *buf_p;
	}
	int rc;
	int i = *size;

	while ((rc = read(fd, buf + i, 1)) > 0) {
		i++;
		if(i >= *a_size){
			*a_size = (*a_size) * 2;
			*buf_p = realloc(*buf_p, (*a_size));
			buf = *buf_p;
			if(buf == NULL)
				break;
		}
	}
	if(i == *size){
		return 0;
	}
	*size = i;
	*buf_p = buf;
	return 1;
}

//parse_text_bin: (int, char *, int, int) -> (int)
// Parser de un buffer en protocolo binario, toma un conjunto de bits, lo separa en argumentos y llama a las funciones
// para llevar a cabo la orden
// Devuelve 0 si consumi todo el buffer, -1 si faltan datos, -2 si no tengo memoria disponible
int parse_text_bin(int fd, char * buf, int buf_size, int index){
	// Si llegue al final del buffer devuelvo 0
	if(index >= buf_size)
		return 0;
	
	if(buf_size == 1 && buf[0] == STATS){
		input_handler_bin(fd, STATS, NULL, NULL, 0, 0);
		return parse_text_bin(fd, buf, buf_size, index + 1);
	}

	// Si tengo menos de 5 bytes no tengo suficientes datos para ejecutar el resto de los comandos
	if(buf_size < 5)
		return -1;
	
	
	int mode = buf[index];
	char * key = NULL;
	char * value = NULL;

	int size = ((int)buf[index+1] * pow(256,3)) + ((int)buf[index+2] * pow(256,2)) + ((int)buf[index+3] * pow(256,1)) + ((int)buf[index+4]);
	int vSize = 0;

	// Si tengo menos bytes de los que indica el size entonces faltan datos
	if((buf_size-5-index) < size)
		return -1;
	
	// Si tengo menos bytes de los que indica el size y ademas esta en modo put y no tengo el tamaño del value entonces faltan datos
	if(mode == PUT && (size + 9) > buf_size)
		return -1;


	key = robust_malloc(sizeof(char)*(size+1), 0);

	if(key == NULL)
		return -2;
	int k = 0;

	for(int i = (index + 5); i < (size+index+5); i++){
		key[k++] = buf[i];
	}
	key[k] = '\0';
	index = (size + index + 5);
	// Si el modo es PUT guardo el value
	if(mode == PUT){
		vSize = ((int)buf[index] * pow(256,3)) + ((int)buf[index+1] * pow(256,2)) + ((int)buf[index+2] * pow(256,1)) + ((int)buf[index+3]);
		
		if((buf_size-4-index) < vSize)
			return -1;
		
		value = robust_malloc(sizeof(char)*(vSize+1), 0);
		
		if(value == NULL)
			return -2;
		
		k = 0;
		
		for(int i = (index + 4); i < (vSize+index+4); i++)
			value[k++] = buf[i];
		
		index = (vSize + index + 4);
	}
	// Llamo al input handle con el comando parseado
	input_handler_bin(fd, mode, key, value, size, vSize);
	// Itero de nuevo para consumir el resto del buffer
	return parse_text_bin(fd, buf, buf_size, index);
}

//input_handler: (int, char**) -> ()
// Handler de input del modo texto, dado los argumentos necesarios para hacer una request, llama a las funciones necesarias
// para ejecutarla, ademas actualiza los valores del STATS
void input_handler(int csock, char ** tok){
	char reply[MAX_RESPONSE];
	int ok = 0;
	if(strcmp(tok[0], "GET") == 0){
		if(tok[1]){
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
	}
	if(strcmp(tok[0], "PUT") == 0){
		if(tok[1] && tok[2]){
			int len = strlen(tok[1]);
			int lenV = strlen(tok[2]);

			char * key = robust_malloc(sizeof(char) * len, 0);
			char * value = robust_malloc(sizeof(char) * lenV, 0);
			if(!key || !value){
				sprintf(reply, "EMEM\n");
			}else{
				memcpy(key, tok[1], len);
				memcpy(value, tok[2], lenV);
				int res = hash_word(key, value, tableSize, strlen(tok[1]), strlen(tok[2]), 0);
				if(res == -1)
					sprintf(reply, "EMEM\n");
				else{
					sprintf(reply, "OK\n");
					pthread_mutex_lock(&putsLock);
					PUTS++;
					pthread_mutex_unlock(&putsLock);
					pthread_mutex_lock(&kvLock);
					KEYVALUES += res;
					pthread_mutex_unlock(&kvLock);
				}
			}
			ok = 1;
		}
	}

	if(strcmp(tok[0], "DEL") == 0){
		if(tok[1]){
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
	}

	if(strcmp(tok[0], "STATS") == 0){
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
	write(csock, reply, strlen(reply));
	return;
}


//handle_conn: (SocketData *) -> (int)
// Handler de la conexion del modo texto, se encarga de llamar a las funciones para parsear y ejecutar un comando
// y dependiendo del resultado modifica o no el buffer correspondiente a ese file descriptor
// Devuelve 1 si se cerro la conexion, 2 si no
int handle_conn(SocketData * event)
{
	int res_text;
	while (1) {
		res_text = text_consume(event->fd, &(event->buf), &(event->index), &(event->size), &(event->a_len));

		switch (res_text){
		case 0:
			close(event->fd);
			return 1;
			break;
		case -1:
			return 2;
			break;
		case -2:
			write(event->fd, "EBIG\n", 5);
			return 2;
			break;
		case -3:
			write(event->fd, "EMEM\n", 5);
			return 2;
			break;
		}

		char ** tokP = parser(event->buf);
		if(tokP == NULL){
			write(event->fd, "EMEM\n", 5);
			return 2;
		}
		
		input_handler(event->fd, tokP);

		for(int i = 0; i < 4; i++){
			if(tokP[i])
        		free(tokP[i]);
		}

		free(tokP);
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

//handle_conn_bin: (SocketData *) -> (int)
// Handler de la conexion del modo binario, se encarga de llamar a las funciones para parsear y ejecutar un comando
// y dependiendo del resultado modifica o no el buffer correspondiente a ese file descriptor
// Devuelve 1 si se cerro la conexion, 2 si no
int handle_conn_bin(SocketData * event)
{
	
	int res, res_text;
	while (1) {

		res_text = text_consume_bin(event->fd, &(event->buf), &(event->index), &(event->size), &(event->a_len));

		if(res_text == 0){
			close(event->fd);
			return 1;
		}
		if(res_text == -1){
			int comm = EMEM;
			write(event->fd, &comm, 1);
			return 2;
		}
		res = parse_text_bin(event->fd, (event->buf), event->size, event->index);
		if(res == -2){
			int comm = EMEM;
			write(event->fd, &comm, 1);
		}
		if(res == 0 || res == -2){
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


//thread_f: (void *) -> (void *)
// Funcion encargada de aceptar y recibir requests de los sockets, hay una ejecucion de esta funcion por thread
void * thread_f(void * arg){
    int nfds;
	int efd, s;
    struct epoll_event events[MAX_EVENTS];
	struct epoll_event event;
    while(1){
        nfds = epoll_wait(epfd, events, 10, -1); 
        for (int i = 0; i < nfds; i++) {
			SocketData *eventData = ((SocketData *)events[i].data.ptr);
			if (lsock == (eventData->fd) || binlsock == (eventData->fd)){ 
				int infd;
				
				if(lsock == (eventData->fd))
					infd = accept (lsock, NULL, NULL);
				else
					infd = accept (binlsock, NULL, NULL);
				
				s = make_socket_non_blocking (infd);
				
				if (s == -1)
					abort();
				
				event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
				SocketData* ctx = robust_malloc(sizeof(SocketData), 0);
				
				if(ctx == NULL){
					close(infd);
					continue;
				}
				
				ctx->fd = infd;
				ctx->bin = binlsock == (eventData->fd) ? 1 : 0;
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
                
			}else{
					int done = 0;
					if((eventData->bin) == 0){
						done = handle_conn(eventData);
					}else{
						done = handle_conn_bin(eventData);
					}


					if (done == 1)
					{
						printf ("Desconectado file descriptor: %d\n",
								(((SocketData*)events[i].data.ptr)->fd));
						close (((SocketData*)events[i].data.ptr)->fd);
						if(events[i].data.ptr){
							if(((SocketData*)events[i].data.ptr)->buf)
								free(((SocketData*)events[i].data.ptr)->buf);
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
    return NULL;
}

//wait_for_clients: () -> ()
// Funcion para levantar un thread por core con la funcion para recibir requests
void wait_for_clients(){
    int number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
    pthread_t clientes[number_of_processors+1];
    for (int i = 0; i < number_of_processors; i++){
		pthread_create(&clientes[i], NULL, thread_f, NULL);
    }
    for (int i = 0; i < number_of_processors; i++){
        pthread_join(clientes[i], NULL);
    }
    return;
}
//mk_lsock: (int) -> (int)
// Crea un socket con el puerto especificado y lo agrega al epoll, devuelve el file descriptor de este
int mk_lsock(int port)
{
	struct sockaddr_in sa;
	int sock;
	int rc;
	int yes = 1;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		quit("socket");

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == 1)
		quit("setsockopt");

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	rc = bind(sock, (struct sockaddr *)&sa, sizeof sa);
	if (rc < 0)
		quit("bind");

	rc = listen(sock, 10);
  
  if(epfd == 0){
  	epfd = epoll_create(1); 
  }
  epoll_ctl_add(epfd, sock, EPOLLIN | EPOLLOUT | EPOLLET);
	
  if (rc < 0)
		quit("listen");

	return sock;
}