#ifndef __SPELL_HASHING_H_
#define __SPELL_HASHING_H_
#include "helpers.h"


//MurmurHash2: (const void *, int, unsigned int) -> (unsigned int)
// Funcion de hasheo basada en el MurmurHash, fuente: https://sites.google.com/site/murmurhash/
unsigned int MurmurHash2 ( const void * key, int len, unsigned int seed );

//hash_first: (char *) -> (unsigned int)
// Toma una palabra y devuelve su hash
unsigned int hash_first(char * word);

//hash_word: (char *, char *, int, int, int, int) -> (int)
// Dados los argumentos de un nodo, crea uno nuevo y lo inserta en la tabla hash
// Devuelve 1 si crea un nodo nuevo, 0 si reemplazo un valor y -1 si no hay mas memoria
int hash_word(char * key, char * value,int counter, int keyLength, int valueLength, int mode);

//find_word: (char *, char **, Word **, int, int) -> (int)
// Busca una palabra en la tabla hash
// devuelve 1 si se encuentra, 0 si no
Word * find_word(char * word, int len);

//find_elem_to_delete: (char*, int) -> (int)
// Elimina el nodo que tiene la key que la funcion toma como argumento
int find_elem_to_delete(char * word, int len);

// initialize_word: (char *, char *, int, int, int, unsigned int) -> (Word *)
// Dados los argumentos necesarios, crea un nuevo nodo para insertar en la tabla hash
Word * initialize_word(char * key, char * value, int keyLength, int valueLength, int mode, unsigned int firstHash);

#endif