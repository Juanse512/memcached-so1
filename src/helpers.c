#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../headers/io.h"
#include "../headers/hashing.h"


Word * insert_word(Word * word, Word * newWord){
    
    set_firstElem(newWord);
    pthread_mutex_lock(&lastElemLock);
    if(lastElemDelete == NULL){
        lastElemDelete = newWord;
    }
    pthread_mutex_unlock(&lastElemLock);

    if(word == NULL){
        word = newWord;
    }else{
        Word * aux = word;
        while(aux != NULL && aux->next != NULL){
            aux = aux->next;
        }
        aux->next = newWord;
        newWord->prev = aux;
    }
    
    return word;
}

void clean_array(Word ** hashTable, int counter){
    for(int i = 0; i < counter; i++)
    {
        hashTable[i] = NULL;
    }
}

void init_lock(pthread_mutex_t* lock, pthread_mutexattr_t* Attr){
    if (pthread_mutex_init(lock, Attr) != 0) {
        printf("\n mutex init failed\n");
        exit(1);
    }
}

void init(){
    struct rlimit lim;
    lim.rlim_cur = 2 * 1024 * 1024 * 8;
    lim.rlim_max = 4 * 1024 * 1024 * 8;

    if(setrlimit(RLIMIT_AS, &lim) == -1)
        fprintf(stderr, "%s\n", "err\n");
    
    long long free_ram = sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGESIZE);
    if(free_ram > 100000){
        free_ram = 100000;
    }

    tableSize = free_ram / (sizeof(Word *) / 4);
    PUTS = 0;
    GETS = 0;
    DELS = 0;
    KEYVALUES = 0; 
    // pthread_mutex_t Mutex;
    pthread_mutexattr_t Attr;

    pthread_mutexattr_init(&Attr);
    pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);

    init_lock(&putsLock, &Attr);
    init_lock(&getsLock, &Attr);
    init_lock(&delsLock, &Attr);
    init_lock(&kvLock, &Attr);
    init_lock(&firstElemLock, &Attr);
    init_lock(&lastElemLock, &Attr);

    firstElemDelete = NULL;
    lastElemDelete = NULL;
    lockSize = tableSize / 10;
    locks = robust_malloc((sizeof(pthread_mutex_t) * lockSize) + 1);
    for(int i = 0; i < lockSize; i++){
        init_lock(&locks[i], &Attr);
    }
    hashTable = robust_malloc(sizeof(Word *) * tableSize);
    clean_array(hashTable, tableSize);
    return;
}


char ** parser(char* str){
    char ** tP = robust_malloc(sizeof(char*)*4);
    char *token; 
    token = strtok(str, " ");
    tP[0] = robust_malloc(sizeof(char) * strlen(token)); 
    strcpy(tP[0], token);
    int i = 1;
    token = strtok(NULL, " ");
    while(token != NULL){
        tP[i] = robust_malloc(sizeof(char) * strlen(token)); 
        strcpy(tP[i++], token);
        token = strtok(NULL, " ");
    }
    return tP;
}


pthread_mutex_t* get_lock(unsigned int position){
    unsigned int pos = (position % lockSize);
    pthread_mutex_t * lock = &locks[pos];
    return lock;
}


void delete_element(Word * actual, unsigned int hash){
    
    if(hashTable[hash % tableSize] == actual)
        hashTable[hash % tableSize] = actual->next;

    Word * next_delete = actual->next_delete;
    Word * prev_delete = actual->prev_delete;
    pthread_mutex_t* prevL = NULL;
    pthread_mutex_t* nextL = NULL;
    printf("prev %p next %p\n", prev_delete, next_delete);
    if(prev_delete){
        unsigned int pos = prev_delete->hash % tableSize;
        prevL = get_lock(pos);
        pthread_mutex_lock(prevL);
        prev_delete->next_delete = next_delete;
        pthread_mutex_unlock(prevL);
    }
    
    if(next_delete){
        unsigned int pos = next_delete->hash % tableSize;
        nextL = get_lock(pos);
        pthread_mutex_lock(nextL);
        next_delete->prev_delete = prev_delete;
        pthread_mutex_unlock(nextL);
    }


    pthread_mutex_lock(&lastElemLock);
    if(actual == lastElemDelete)
        lastElemDelete = prev_delete;
    
    pthread_mutex_unlock(&lastElemLock);

    pthread_mutex_lock(&firstElemLock);
    if(actual == firstElemDelete){
        firstElemDelete = actual->next_delete;
        firstElemDelete->prev_delete = NULL;
    }
    pthread_mutex_unlock(&firstElemLock);
    if(actual->prev)
        actual->prev->next = actual->next;
    if(actual->next)
        actual->next->prev = actual->prev;
    
    
    
        

    free(actual->word.string);
    free(actual->value.string);
    free(actual);
    return;
}


int compare_string(CompString s1, CompString s2){
    if(s1.len != s2.len)
        return 1;
    char * word1 = s1.string;
    char * word2 = s2.string;
    int res = 0;
    for(int i = 0; i < s1.len; i++){
        if(word1[i] != word2[i])
            res = 1;
    }
    return res;
}


void freeMemory(){
    pthread_mutex_lock(&lastElemLock);
    if(lastElemDelete == NULL){
        pthread_mutex_unlock(&lastElemLock);
        exit(1);
        return;
    }

    Word * aux = lastElemDelete->prev_delete;
    Word * elem_to_delete = lastElemDelete;
    unsigned int hash = hash_first(elem_to_delete->word.string);
    pthread_mutex_t* lock = get_lock(hash % tableSize);
    
    while(pthread_mutex_trylock(lock) == -1){
        if(elem_to_delete->prev_delete){
            elem_to_delete = elem_to_delete->prev_delete;
            hash = hash_first(elem_to_delete->word.string);
            lock = get_lock(hash % tableSize);
            aux = elem_to_delete->prev_delete;
        }else{
            elem_to_delete = NULL;
            break;
        }
    }

    if(elem_to_delete == NULL){
        pthread_mutex_unlock(&lastElemLock);
        exit(1);
        return;
    }

    if(elem_to_delete == hashTable[hash % tableSize])
        hashTable[hash % tableSize] = hashTable[hash % tableSize]->next;
    

    if(elem_to_delete->prev)
        elem_to_delete->prev->next = elem_to_delete->next;
    
    if(elem_to_delete->next)
        elem_to_delete->next->prev = elem_to_delete->prev;
    
    if(elem_to_delete == lastElemDelete)
        lastElemDelete = aux;
    else{
        if(elem_to_delete->prev_delete)
            elem_to_delete->prev_delete->next_delete = elem_to_delete->next_delete->prev_delete;
        
        if(elem_to_delete->next_delete)
            elem_to_delete->next_delete->prev_delete = elem_to_delete->prev_delete->next_delete;
    }

    free(elem_to_delete->word.string);
    free(elem_to_delete->value.string);
    free(elem_to_delete);

    pthread_mutex_unlock(lock);
    pthread_mutex_unlock(&lastElemLock);
    return;
}



void * robust_malloc(size_t size){
    void * pointer = malloc(size);
    while(pointer == NULL){
        freeMemory();
        pointer = malloc(size);
    }
    return pointer;
}

void set_firstElem(Word * element){
    pthread_mutex_lock(&firstElemLock);
    if(firstElemDelete){
        firstElemDelete->prev_delete = element;
    }
    element->next_delete = firstElemDelete;
    element->prev_delete = NULL;
    firstElemDelete = element;
    pthread_mutex_unlock(&firstElemLock);
    return;
}