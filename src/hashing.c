#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <pthread.h>
#include "../headers/io.h"
#include "../headers/helpers.h"

//MurmurHash2: (const void *, int, unsigned int) -> (unsigned int)
// Funcion de hasheo basada en el MurmurHash, fuente: https://sites.google.com/site/murmurhash/
unsigned int MurmurHash2 ( const void * key, int len, unsigned int seed ){
	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	unsigned int h = seed ^ len;

	const unsigned char * data = (const unsigned char *)key;

	while(len >= 4)
	{
		unsigned int k = *(unsigned int *)data;

		k *= m; 
		k ^= k >> r; 
		k *= m; 
		
		h *= m; 
		h ^= k;

		data += 4;
		len -= 4;
	}
	
	switch(len)
	{
	case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0];
	        h *= m;
	};


	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
} 

//hash_first: (char *) -> (unsigned int)
// Toma una palabra y devuelve su hash
unsigned int hash_first(char * word){
    
    int len = strlen(word);
    
    unsigned int hash = MurmurHash2(word, len, SEED_HASH);
    
    return hash;
}

//find_word: (char *, char **, Word **, int, int) -> (int)
// Busca una palabra en la tabla hash
// devuelve 1 si se encuentra, 0 si no
Word * find_word(char * word, int len){
    unsigned int first_hash = hash_first(word);
    unsigned int position = first_hash % tableSize;
    int flag = 1;
    // Obtengo el lock correspondiente a la zona de la posicion dada
    pthread_mutex_t* lock = get_lock(position);

    pthread_mutex_lock(lock);

    Word * aux = hashTable[position];
    Word * returnValue = NULL;
    // Mientras no haya encontrado el valor y no haya llegado al final de la lista
    while(aux != NULL && flag != 0){
        if(aux->hash == first_hash){
            if(aux->word.string != NULL){ 
                CompString wordString;
                wordString.string = word;
                wordString.len = len;
                // Comparo los hashes y luego los strings de la key a buscar y el nodo
                if(compare_string(wordString, aux->word) == 0){
                    returnValue = aux;

                    pthread_mutex_t* next_lock = returnValue->next_delete ? get_lock(returnValue->next_delete->hash % tableSize) : NULL;
                    pthread_mutex_t* prev_lock = returnValue->prev_delete ? get_lock(returnValue->prev_delete->hash % tableSize) : NULL;
                    
                    pthread_mutex_lock(&lastElemLock);
                    // Si el elemento encontrado es el tope de la lista, lo reemplazo por el siguiente elemento a eliminar
                    if(returnValue == lastElemDelete)
                        lastElemDelete = returnValue->prev_delete;
                    pthread_mutex_unlock(&lastElemLock);

                    if(prev_lock)
                        pthread_mutex_lock(prev_lock);

                    if(next_lock)
                        pthread_mutex_lock(next_lock);

                    // Saco el elemento encontrado de la cola
                    if(returnValue->next_delete)
                        returnValue->next_delete->prev_delete = returnValue->prev_delete;
                    
                    if(returnValue->prev_delete)
                        returnValue->prev_delete->next_delete = returnValue->next_delete;
                    if(next_lock)
                        pthread_mutex_unlock(next_lock);

                    if(prev_lock)
                        pthread_mutex_unlock(prev_lock);

                    // Lo pongo como ultimo elemento a eliminar
                    set_firstElem(returnValue);
                    // Cambio la flag para salir del while
                    flag = 0;
                }
            }
        }
        aux = aux->next;
    }
    pthread_mutex_unlock(lock);
    return returnValue;
}

// initialize_word: (char *, char *, int, int, int, unsigned int) -> (Word *)
// Dados los argumentos necesarios, crea un nuevo nodo para insertar en la tabla hash
Word * initialize_word(char * key, char * value, int keyLength, int valueLength, int mode, unsigned int firstHash){
    Word * newWord = robust_malloc(sizeof(Word), 0);
    if(newWord == NULL)
        return NULL;
    newWord->next = NULL;
    newWord->prev = NULL;
    newWord->hash = firstHash;

    newWord->word.string = key;
    newWord->word.len = keyLength;

    newWord->value.string = value;
    newWord->value.len = valueLength;
    newWord->bin = mode;
    return newWord;
}

//hash_word: (char *, char *, int, int, int, int) -> (int)
// Dados los argumentos de un nodo, crea uno nuevo y lo inserta en la tabla hash
// Devuelve 1 si crea un nodo nuevo, 0 si reemplazo un valor y -1 si no hay mas memoria
int hash_word(char * key, char * value,int counter, int keyLength, int valueLength, int mode){
    int ret = 0;
    Word * foundPos = find_word(key, keyLength);
    // Si la key no esta en la tabla hash, creo un nodo nuevo y lo inserto, si esta reemplazo el value
    if(foundPos == NULL){
        unsigned int firstHash = 0, position = 0;
        
        firstHash = hash_first(key);
        position = firstHash % (unsigned int)counter;

        Word * newWord = initialize_word(key, value, keyLength, valueLength, mode, firstHash);
        if(newWord == NULL)
            return -1;
        pthread_mutex_t* lock = get_lock(position);
        
        pthread_mutex_lock(lock);
        hashTable[position] = insert_word(hashTable[position], newWord);
        pthread_mutex_unlock(lock);
        
        ret = 1;
    }else{
        free(foundPos->value.string);
        foundPos->value.string = value;
        foundPos->value.len = valueLength;
        foundPos->bin = mode;
        free(key);
        ret = 0;
    }

    return ret;
}

//find_elem_to_delete: (char*, int) -> (int)
// Elimina el nodo que tiene la key que la funcion toma como argumento
int find_elem_to_delete(char * word, int len){
    Word * foundPos = find_word(word, len);
    if(foundPos){
        unsigned int pos = foundPos->hash % tableSize; 
        pthread_mutex_t* lock = get_lock(pos);
        pthread_mutex_lock(lock);
        delete_element(foundPos, foundPos->hash);
        pthread_mutex_unlock(lock);
        return 1;
    }else{
        return 0;
    }
}
