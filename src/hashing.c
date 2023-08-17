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

Word * find_word(char * word, int len){
    unsigned int first_hash = hash_first(word);
    unsigned int position = first_hash % tableSize;
    int flag = 1;
    pthread_mutex_t* lock = get_lock(position);

    pthread_mutex_lock(lock);

    Word * aux = hashTable[position];
    Word * returnValue = NULL;
    while(aux != NULL && flag != 0){
        if(aux->hash == first_hash){
            if(aux->word.string != NULL){ 
                CompString wordString;
                wordString.string = word;
                wordString.len = len;
                if(compare_string(wordString, aux->word) == 0){
                    returnValue = aux;
                    printf("HERE %s\n", returnValue->value.string);
                    if(returnValue->next_delete) /// aca
                        printf("next %u\n", returnValue->next_delete->hash);
                    if(returnValue->prev_delete)
                        printf("prev %u\n", returnValue->prev_delete->hash);
                    pthread_mutex_t* next_lock = returnValue->next_delete ? get_lock(returnValue->next_delete->hash % tableSize) : NULL;
                    pthread_mutex_t* prev_lock = returnValue->prev_delete ? get_lock(returnValue->prev_delete->hash % tableSize) : NULL;
                    
                    pthread_mutex_lock(&lastElemLock);
                    if(returnValue == lastElemDelete)
                        lastElemDelete = returnValue->prev_delete;
                    pthread_mutex_unlock(&lastElemLock);

                    if(prev_lock)
                        pthread_mutex_lock(prev_lock);

                    if(next_lock)
                        pthread_mutex_lock(next_lock);

                    if(returnValue->next_delete)
                        returnValue->next_delete->prev_delete = returnValue->prev_delete;
                    
                    if(returnValue->prev_delete)
                        returnValue->prev_delete->next_delete = returnValue->next_delete;
                    if(next_lock)
                        pthread_mutex_unlock(next_lock);

                    if(prev_lock)
                        pthread_mutex_unlock(prev_lock);
                    
                    set_firstElem(returnValue);
                    
                    flag = 0;
                }
            }
        }
        aux = aux->next;
    }
    pthread_mutex_unlock(lock);
    return returnValue;
}

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


int hash_word(char * key, char * value,int counter, int keyLength, int valueLength, int mode){
    int ret = 0;
    Word * foundPos = find_word(key, keyLength);

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
        ret = 1;
    }

    return ret;
}

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
