#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <pthread.h>
#include "../headers/io.h"
#include "../headers/helpers.h"
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

unsigned int hash_first(char * word){
    
    int len = strlen(word);
    
    unsigned int hash = MurmurHash2(word, len, SEED_HASH);
    
    return hash;
}


// Word ** hash_words(char * dictionary[], int counter, int * size){
//     int tableSize = ceil(counter / 0.7); // factor de carga 0.7
    
//     Word ** hashTable = malloc(sizeof(Word *) * (tableSize + 1));
    
//     clean_array(hashTable, tableSize);
    
//     unsigned int firstHash = 0, position = 0;
    
//     for(int i = 0; i < counter; i++){
//         firstHash = hash_first(dictionary[i]);
        
//         position = firstHash % tableSize;
        
//         hashTable[position] = insert_word(hashTable[position], firstHash, NULL);
//     }
//     *size = tableSize;
//     return hashTable;
// }





Word * find_word(char * word){
    unsigned int first_hash = hash_first(word);
    unsigned int position = first_hash % tableSize;
    int flag = 1;
    pthread_mutex_t* lock = get_lock(position);
    printf("%p\n", lock);
    pthread_mutex_lock(lock);
    Word * aux = hashTable[position];
    Word * returnValue = NULL;
    while(aux != NULL && flag != 0){
        if(aux->hash == first_hash){
            if(aux->word != NULL){ 
                if(strcmp(word, aux->word) == 0){
                    returnValue = aux;
                    if(returnValue->prev_delete != NULL){
                        pthread_mutex_lock(&lastElemLock);
                        if(returnValue == lastElemDelete){
                            lastElemDelete = returnValue->prev_delete;
                        }
                        pthread_mutex_unlock(&lastElemLock);

                        pthread_mutex_t* prev_lock = get_lock(returnValue->prev_delete->hash % tableSize);
                        pthread_mutex_t* next_lock = get_lock(returnValue->next_delete->hash % tableSize);
                        // PUEDE ENTRAR EN DEADLOCK? CREO QUE NO
                        if(prev_lock != lock){
                            pthread_mutex_lock(prev_lock);
                        }
                        if(next_lock != lock){
                            pthread_mutex_lock(next_lock);
                        }
                            returnValue->prev_delete->next_delete = returnValue->next_delete;
                        if(next_lock != lock){
                            pthread_mutex_unlock(next_lock);
                        }
                        if(prev_lock != lock){
                            pthread_mutex_unlock(prev_lock);
                        }
                    }
                    returnValue->prev_delete = NULL;
                    pthread_mutex_lock(&firstElemLock);
                    returnValue->next_delete = firstElemDelete;
                    firstElemDelete = returnValue;
                    pthread_mutex_unlock(&firstElemLock);
                    flag = 0;
                }
            }
        }
        aux = aux->next;
    }
    pthread_mutex_unlock(lock);
    return returnValue;
}

int hash_word(char * key, char * value,int counter){
    // int tableSize = ceil(counter / 0.7); // factor de carga 0.7
    
    // Word ** hashTable = malloc(sizeof(Word *) * (tableSize + 1));
    
    // clean_array(hashTable, tableSize);
    int ret = 0;
    Word * foundPos = find_word(key);
    if(foundPos == NULL){
        unsigned int firstHash = 0, position = 0;
        
        firstHash = hash_first(key);
        position = firstHash % counter;
        pthread_mutex_t* lock = get_lock(position);
        pthread_mutex_lock(lock);
        hashTable[position] = insert_word(hashTable[position], firstHash, key, value);
        pthread_mutex_unlock(lock);
        ret = 1;
    }else{
        foundPos->value = value;
    }
    
    // *size = tableSize;
    return ret;
}

int find_elem_to_delete(char * word){
    unsigned int first_hash = hash_first(word);
    unsigned int position = first_hash % tableSize;
    int flag = 0;
    pthread_mutex_t* lock = get_lock(position);
    pthread_mutex_lock(lock);
    Word * aux = hashTable[position];
    Word * returnValue = NULL;
    printf("%p\n", aux);
    if(aux){
        if(aux->hash == first_hash){
            if(aux->word != NULL){ 
                    if(strcmp(word, aux->word) == 0){
                        hashTable[position] = aux->next;
                        free(aux->word);
                        free(aux->value);
                        free(aux);
                        flag = 1;
                    }
                }
        }else{
            while(aux->next != NULL){
                if(aux->next->hash == first_hash){
                    if(aux->next->word != NULL){ 
                        if(strcmp(word, aux->next->word) == 0){
                            delete_element(aux, aux->next);
                            flag = 1;
                            break;
                        }
                    }
                }
                aux = aux->next;
            }

        }
    }
    pthread_mutex_unlock(lock);
    return flag;
    
}
