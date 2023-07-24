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





Word * find_word(char * word, int len){
    unsigned int first_hash = hash_first(word);
    unsigned int position = first_hash % tableSize;
    int flag = 1;
    pthread_mutex_t* lock = get_lock(position);
    printf("%p\n", lock);
    pthread_mutex_lock(lock);
    Word * aux = hashTable[position];
    Word * returnValue = NULL;
    printf("hash: %ld", strlen(word));
    while(aux != NULL && flag != 0){
        if(aux->hash == first_hash){
            if(aux->word.string != NULL){ 
                CompString wordString;
                wordString.string = word;
                wordString.len = len;
                if(compare_string(wordString, aux->word) == 0){
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
                            // revisar si next lock y prev lock son iguales
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

int hash_word(char * key, char * value,int counter, int keyLength, int valueLength, int mode){
    // int lengthValue = strlen(value);
    // int tableSize = ceil(counter / 0.7); // factor de carga 0.7
    
    // Word ** hashTable = malloc(sizeof(Word *) * (tableSize + 1));
    
    // clean_array(hashTable, tableSize);
    int ret = 0;
    Word * foundPos = find_word(key, keyLength);
    if(foundPos == NULL){
        unsigned int firstHash = 0, position = 0;
        
        firstHash = hash_first(key);
        position = firstHash % counter;
        pthread_mutex_t* lock = get_lock(position);
        pthread_mutex_lock(lock);
        hashTable[position] = insert_word(hashTable[position], firstHash, key, value, keyLength, valueLength, mode);
        pthread_mutex_unlock(lock);
        ret = 1;
    }else{
        printf("FOUND MATCH %s\n", value);
        free(foundPos->value.string);
        foundPos->value.string = malloc(sizeof(char)*valueLength);
        memcpy(foundPos->value.string, value, valueLength);
        foundPos->value.len = valueLength;
        foundPos->bin = mode;
        // foundPos->value = value;
        
        //free del value viejo
        ret = 1;
    }
    
    // *size = tableSize;
    return ret;
}

int find_elem_to_delete(char * word, int len){
    unsigned int first_hash = hash_first(word);
    unsigned int position = first_hash % tableSize;
    int flag = 0;
    pthread_mutex_t* lock = get_lock(position);
    pthread_mutex_lock(lock);
    Word * aux = hashTable[position];
    Word * returnValue = NULL;
    CompString wordString;
    wordString.string = word;
    wordString.len = len;
    printf("%p\n", aux);
    if(aux){
        if(aux->hash == first_hash){
            if(aux->word.string != NULL){ 
                    
                    if(compare_string(wordString, aux->word) == 0){
                        hashTable[position] = aux->next;
                        free(aux->word.string);
                        free(aux->value.string);
                        free(aux);
                        flag = 1;
                    }
                }
        }else{
            while(aux->next != NULL){
                if(aux->next->hash == first_hash){
                    if(aux->next->word.string != NULL){ 
                        if(compare_string(wordString, aux->next->word) == 0){
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
