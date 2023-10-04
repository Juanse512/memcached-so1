#ifndef __NETWORKING_IO_H_
#define __NETWORKING_IO_H_
#include <stdint.h>


typedef struct BinaryStruct{
  int mode;
  int keySize;
  char * key;
  int valueSize;
  char * value;
  int ind;
} Binary;
typedef struct SocketDataS {
    int fd; // file descriptor del socket
    int bin; // flag para saber si es binario
	char * buf; // buffer de entrada
	int index; // indice en el buffer
	int size; // bytes usados del buffer
	int a_len; // tamaño total del buffer
	Binary * binary; 
} SocketData;

int READ(int fd, char * buf, int n);

//epoll_ctl_add: (int, int, uint32_t) -> ()
// Agrega un socket a la instancia de epoll
void epoll_ctl_add(int epfd, int fd, uint32_t events);

//make_socket_non_blocking: (int) -> (int)
// Toma un file descriptor y hace el socket no bloqueante
int make_socket_non_blocking (int sfd);

//read_byte(int, char*) -> (int)
// Lee un byte de un file descriptor y lo guarda en el char* byte
int read_byte(int fd, char* byte);

//inpunt_handler_bin: (int, int, char*, char*, int, int) -> ()
// Handler de input del modo binario, dado los argumentos necesarios para hacer una request, llama a las funciones necesarias
// para ejecutarla, ademas actualiza los valores del STATS
void input_handler_bin(int csock, int mode, char* key, char* val, int keyLen, int valLen);

//text_consume: (int, char **, int *, int *, int *) -> (int)
// Consume el buffer del file descriptor en modo texto
// Devuelve -1 si no se termino de consumir un comando entero, -2 si el comando es demasiado grande para este modo
// -3 si no tengo memoria disponible, 0 si no hay nada que leer y 1 si se pudo leer y no hubo problemas
int text_consume(int fd, char ** buf_p, int * index, int * size, int * a_size);

//text_consume_bin: (int, char **, int *, int *, int *) -> (int)
// Consume el buffer del file descriptor en modo binario
// Devuelve -1 si no hay memoria disponible, 0 si no hay nada que leer y 1 si todo salio correctamente
int text_consume_bin(int fd, char ** buf_p, int * index, int * size, int * a_size, Binary * binary);

//input_handler: (int, char**) -> ()
// Handler de input del modo texto, dado los argumentos necesarios para hacer una request, llama a las funciones necesarias
// para ejecutarla, ademas actualiza los valores del STATS
void input_handler(int csock, char ** tok);

//handle_conn: (SocketData *) -> (int)
// Handler de la conexion del modo texto, se encarga de llamar a las funciones para parsear y ejecutar un comando
// y dependiendo del resultado modifica o no el buffer correspondiente a ese file descriptor
// Devuelve 1 si se cerro la conexion, 2 si no
int handle_conn(SocketData * event);

//handle_conn_bin: (SocketData *) -> (int)
// Handler de la conexion del modo binario, se encarga de llamar a las funciones para parsear y ejecutar un comando
// y dependiendo del resultado modifica o no el buffer correspondiente a ese file descriptor
// Devuelve 1 si se cerro la conexion, 2 si no
int handle_conn_bin(SocketData * event);

//thread_f: (void *) -> (void *)
// Funcion encargada de aceptar y recibir requests de los sockets, hay una ejecucion de esta funcion por thread
void * thread_f(void * arg);

//wait_for_clients: () -> ()
// Funcion para levantar un thread por core con la funcion para recibir requests
void wait_for_clients();

//mk_lsock: (int) -> (int)
// Crea un socket con el puerto especificado y lo agrega al epoll, devuelve el file descriptor de este
int mk_lsock(int port);

#endif