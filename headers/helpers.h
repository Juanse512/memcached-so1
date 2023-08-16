#ifndef __SPELL_HELPERS_H_
#define __SPELL_HELPERS_H_
#define INITIAL_LEN 100
#define MAX_LEN 10000
#define SEED_HASH 13
#define MAX_RESPONSE 2048

//Word:
// index: indica el indice de la palabra en el diccionario en la tabla hash original y la distancia a la palabra original en la tabla hash de palabras ya calculadas
// word: contiene la palabra, no se una en la tabla hash original del diccionario
// hash: hash de la palabra
// next: puntero al Word siguiente

typedef struct CompStringStruct{
  char * string;
  int len;
} CompString;


typedef struct WordStruct{
  CompString word;
  CompString value;
  int bin;
  unsigned int hash;
  struct WordStruct* next_delete;
  struct WordStruct* prev_delete;
  struct WordStruct* next;
  struct WordStruct* prev;
} Word;


int tableSize;
int lockSize;
int binlsock, lsock;
int PUTS, DELS, GETS, KEYVALUES;
pthread_mutex_t putsLock, delsLock, getsLock, kvLock, firstElemLock, lastElemLock;
Word * lastElemDelete;
Word * firstElemDelete;
Word ** hashTable;

pthread_mutex_t * locks;

//clean_array: (Word **, int) -> ()
// Toma un array de punteros y los pone a NULL
void clean_array(Word ** hashTable, int counter);

//insert_word: (int, Word *,unsigned int, char *) -> (Word*)
// Toma una palabra o el indice de una y la coloca al final de la Word que le pasamos, devuelve la Word actualizada
Word * insert_word(Word * word, Word * newWord);

//save_word: (char *, char **, int) -> ()
// Guarda una palabra en el array que le pasamos, en la posicion dada
// void save_word(char * word, char * dictionary[], int index);

// save_word: (char ***, int, int) -> (char***)
// Dado el puntero de un array de punteros y un contador, si el contador es mayor al tamaño del array, se triplica el tamaño de este y devuelve el puntero modificado
// free_list: (Word *) -> ()
// Libera la lista enlazada de un Word
void free_list(Word * word);

// free_all: (char **, Word **, int, int) -> ()
// Libera el diccionario y la tabla hash
void free_all(Word ** hashTable);

void init();

char ** parser(char* str);

pthread_mutex_t* get_lock(unsigned int position);

void delete_element(Word * prev, Word * actual);

int compare_string(CompString s1, CompString s2);

void freeMemory();
#endif