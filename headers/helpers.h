#ifndef __SPELL_HELPERS_H_
#define __SPELL_HELPERS_H_
#define INITIAL_LEN 100
#define MAX_LEN 10000
#define SEED_HASH 13
#define MAX_RESPONSE 2048
// Modificar estos dos valores para limitar la memoria 
#define MAX_MEMORY_CUR 2 * 1024 * 1024 * 8
#define MAX_MEMORY_MAX 4 * 1024 * 1024 * 8
// Este valor limita el tamaño que puede tomar la tabla hash, no la memoria total
#define MAX_FREE_RAM 100000
#define MAX_EVENTS 10 // Maximos eventos del epoll
typedef struct CompStringStruct{
  char * string; // cadena de caracteres
  int len;   // largo de la cadena
} CompString;


typedef struct WordStruct{
  CompString word; // clave del nodo
  CompString value; // valor del nodo
  int bin; // flag que indica si es binario
  unsigned int hash; // hash de la clave
  struct WordStruct* next_delete; // elemento siguiente a eliminar luego de este 
  struct WordStruct* prev_delete; // elemento anterior a eliminar antes de este
  struct WordStruct* next; // siguiente elemento en la lista enlazada
  struct WordStruct* prev; // elemento anterior en la lista enlazada
} Word;


extern int tableSize; // Tamaño de la tabla
extern int lockSize; // Tamaño del array de locks
extern int binlsock, lsock; // sockets binario y texto (889 y 888 respectivamente)
extern int PUTS, DELS, GETS, KEYVALUES; // Valores de stats
extern pthread_mutex_t putsLock, delsLock, getsLock, kvLock, firstElemLock, lastElemLock;
// Locks de los valores de stats y de la cola de borrado
extern Word * lastElemDelete; // Ultimo elemento de la cola de borrado, primero a eliminar
extern Word * firstElemDelete; // Primer elemento de la cola, ultimo a eliminar
extern Word ** hashTable; // Tabla hash

pthread_mutex_t * locks; // Array de locks, cada lock abarca varias columnas de la tabla hash

//clean_array: (Word **, int) -> ()
// Toma un array de punteros y los pone a NULL
void clean_array(Word ** hashTable, int counter);

//insert_word: (Word *, Word *) -> (Word*)
// Toma el primer elemento de una lista y un nuevo nodo e inserta este al final de la cola
Word * insert_word(Word * word, Word * newWord);

// init_lock: (pthread_mutex_t*, pthread_mutexattr_t*) -> ()
// Dado un lock y una struct de atributos, inicializa el lock
void init_lock(pthread_mutex_t* lock, pthread_mutexattr_t* Attr);

// init: () -> ()
// Inicializa todas las variables necesarias para que el programa funcione
void init();

//parser: (char*) -> (char **)
// Dado un string, lo separa en espacios y devuelve un nuevo array de arrays con estos valores
char ** parser(char* str);

//get_lock: (unsigned int) -> (pthread_mutex_t*)
// Dada una posicion, devuelve el lock que le corresponde
pthread_mutex_t* get_lock(unsigned int position);

//delete_element: (Word*, unsigned int) -> ()
// Dado un elemento y su hash borra el elemento y conecta los nodos relacionados
void delete_element(Word * actual, unsigned int hash);

//compare_string: (CompString, CompString) -> (int)
// Nueva funcion de comparacion para elementos del modo binario, a prueba de \n
// Devuelve 0 si son iguales, 1 si no
int compare_string(CompString s1, CompString s2);

//free_memory: () -> (int)
// Se encarga de eliminar un elemento de la cola de borrado cuando no hay memoria disponible
// Devuelve 1 si elimino correctamente, -1 si no
int free_memory();

//robust_malloc(size_t, int) -> void *
// Dado un tamaño y una flag de init, hace un malloc que solo devuelve NULL si no tiene memoria y no puede liberar mas
// La flag de init indica si ese malloc es parte de la inicializacion del programa, si lo es y no hay memoria el programa se detiene
// ya que no tiene memoria suficiente para inciar, si no devuelve NULL y es handleado por la funcion que lo llama
void * robust_malloc(size_t size, int init);

//set_firstElem: (Word *) -> ()
// Toma un nodo y lo coloca como ultimo elemento a borrar en la cola de borrado
void set_firstElem(Word * element);
#endif