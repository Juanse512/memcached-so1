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

void init();

char ** parser(char* str);

pthread_mutex_t* get_lock(unsigned int position);

void delete_element(Word * actual, unsigned int hash);

int compare_string(CompString s1, CompString s2);

void freeMemory();

void * robust_malloc(size_t size);

void set_firstElem(Word * element);
#endif