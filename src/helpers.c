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

void free_all(Word ** hashTable){   
    for(int i = 0; i < tableSize; i++){
        free_list(hashTable[i]);
    }
    pthread_mutex_destroy(&putsLock);
    pthread_mutex_destroy(&delsLock);
    pthread_mutex_destroy(&getsLock);
    pthread_mutex_destroy(&kvLock);
    pthread_mutex_destroy(&firstElemLock);
    pthread_mutex_destroy(&lastElemLock);
    for(int i = 0; i < lockSize; i++){
        pthread_mutex_destroy(&locks[i]);
    }
    free(hashTable);
    free(locks);

}

void free_list(Word * word){
    if(word == NULL)
        return;
    if(word->word.string != NULL) free(word->word.string);
    if(word->value.string != NULL) free(word->value.string);
    Word * next = word->next;
    free(word);
    free_list(next);
}


// void save_word(char * word, char * dictionary[], int index){
//     dictionary[index] = malloc(sizeof(char) * (strlen(word) + 1));
//     strcpy(dictionary[index], word);
// }

Word * insert_word(Word * word, Word * newWord){
    // Word * newWord = malloc(sizeof(Word));
    // while(newWord == NULL){
    //     word = freeMemory(&hash);
    //     printf("FREE MEMORY INSERT\n");
    //     newWord = malloc(sizeof(Word));
    // }
    // newWord->next = NULL;
    // newWord->prev = NULL;
    // newWord->hash = hash;
    // // newWord->word.string = malloc(sizeof(char)*lengthWord);
    // // memcpy(newWord->word.string, wordChar, lengthWord);
    // newWord->word.string = wordChar;
    // newWord->word.len = lengthWord;
    // // newWord->value.string = malloc(sizeof(char)*lengthValue);
    // // memcpy(newWord->value.string, value, lengthValue);
    // newWord->value.string = value;
    // newWord->value.len = lengthValue;
    // newWord->bin = mode;
    
    pthread_mutex_lock(&firstElemLock);
    if(firstElemDelete){
        firstElemDelete->prev_delete = newWord;
    }
    newWord->next_delete = firstElemDelete;
    firstElemDelete = newWord;
    pthread_mutex_unlock(&firstElemLock);

    newWord->prev_delete = NULL;
    

    pthread_mutex_lock(&lastElemLock);
    if(lastElemDelete == NULL){
        lastElemDelete = newWord;
    }
    pthread_mutex_unlock(&lastElemLock);

    if(word == NULL){
        word = newWord;
    }else{
        Word * aux = word;
        printf("PRE WHILE %p\n", aux);
        while(aux != NULL && aux->next != NULL){
            printf("aux %p\n", aux);

            aux = aux->next;
        }
        aux->next = newWord;
        newWord->prev = aux;
        printf("AFTER WHILE %p %p\n", aux, aux->next);
    }
    
    return word;
}

void clean_array(Word ** hashTable, int counter){
    for(int i = 0; i < counter; i++)
    {
        hashTable[i] = NULL;
    }
}


void init(){
    struct rlimit lim;
    if (getrlimit(RLIMIT_AS, &lim) == -1) {
        perror("getrlimit");
        exit(EXIT_FAILURE);
    }

    printf("Current resource limits:\n");
    printf("Soft limit (RLIMIT_AS): %lu\n", lim.rlim_cur);
    printf("Hard limit (RLIMIT_AS): %lu\n", lim.rlim_max);
    lim.rlim_cur = 2 * 1024 * 1024 * 8;
    lim.rlim_max = 4 * 1024 * 1024 * 8;
    if(setrlimit(RLIMIT_AS, &lim) == -1)
        fprintf(stderr, "%s\n", "err\n");
    if (getrlimit(RLIMIT_AS, &lim) == -1) {
        perror("getrlimit");
        exit(EXIT_FAILURE);
    }

    printf("Current resource limits:\n");
    printf("Soft limit (RLIMIT_AS): %lu\n", lim.rlim_cur);
    printf("Hard limit (RLIMIT_AS): %lu\n", lim.rlim_max);
    long long free_ram = sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGESIZE);
    if(free_ram > 800){
        free_ram = 800;
    }
    printf("FREE RAM %lld\n", free_ram);
    tableSize = free_ram / (sizeof(Word *) / 4);
    PUTS = 0;
    GETS = 0;
    DELS = 0;
    KEYVALUES = 0; 
    // pthread_mutex_t Mutex;
    pthread_mutexattr_t Attr;

    pthread_mutexattr_init(&Attr);
    pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&putsLock, &Attr) != 0) {
        printf("\n mutex init failed\n");
        return;
    }
    if (pthread_mutex_init(&getsLock, &Attr) != 0) {
        printf("\n mutex init failed\n");
        return;
    }
    if (pthread_mutex_init(&delsLock, &Attr) != 0) {
        printf("\n mutex init failed\n");
        return;
    }
    if (pthread_mutex_init(&kvLock, &Attr) != 0) {
        printf("\n mutex init failed\n");
        return;
    }
    if (pthread_mutex_init(&firstElemLock, &Attr) != 0) {
        printf("\n mutex init failed\n");
        return;
    }
    if (pthread_mutex_init(&lastElemLock, &Attr) != 0) {
        printf("\n mutex init failed\n");
        return;
    }
    firstElemDelete = NULL;
    lastElemDelete = NULL;
    // tableSize = 100;
    lockSize = tableSize / 10;
    locks = malloc((sizeof(pthread_mutex_t) * lockSize) + 1);
    for(int i = 0; i < lockSize; i++){
        if (pthread_mutex_init(&locks[i], &Attr) != 0) {
            printf("\n mutex init failed\n");
            return;
        }
    }
    hashTable = malloc(sizeof(Word *) * tableSize);
    clean_array(hashTable, tableSize);
    return;
}


char ** parser(char* str){
    // const char s[2] = " ";
    char ** tP = malloc(sizeof(char*)*4);
    char *token; 
    token = strtok(str, " ");
    tP[0] = malloc(sizeof(char) * strlen(token)); 
    strcpy(tP[0], token);
    // strcpy(tok[0], token);
    int i = 1;
    token = strtok(NULL, " ");
    while(token != NULL){
        tP[i] = malloc(sizeof(char) * strlen(token)); 
        strcpy(tP[i++], token);
        // strcpy(tok[i++], token);
        token = strtok(NULL, " ");
    }
    return tP;
}


pthread_mutex_t* get_lock(unsigned int position){

    unsigned int pos = (position % lockSize);
    pthread_mutex_t * lock = &locks[pos];
    return lock;
}


void delete_element(Word * prev, Word * actual){
    pthread_mutex_lock(&lastElemLock);
    if(actual == lastElemDelete){
        lastElemDelete = actual->prev_delete;
        
    }
    pthread_mutex_unlock(&lastElemLock);
    pthread_mutex_lock(&firstElemLock);
    if(actual == firstElemDelete){
        firstElemDelete = actual->next_delete;
    }
    pthread_mutex_unlock(&firstElemLock);
    prev->next = actual->next;
    if(actual->next){
        actual->next->prev = prev;
    }
    if(actual->prev){
        actual->prev->next = actual->next;
    }
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
    if(lastElemDelete){
        Word * aux = lastElemDelete->prev_delete;
        Word * elem_to_delete = lastElemDelete;
        unsigned int hash = hash_first(elem_to_delete->word.string);
        pthread_mutex_t* lock = get_lock(hash % tableSize);
        while(pthread_mutex_trylock(lock) == -1){
            printf("IN TRYLOCK\n\n\n\n\n\n\n");
            if(elem_to_delete->prev_delete){
                elem_to_delete = elem_to_delete->prev_delete;
                hash = hash_first(elem_to_delete->word.string);
                lock = get_lock(hash % tableSize);
                aux = elem_to_delete->prev_delete;
            }else{
                exit(1);
            }
        }
        
        if(elem_to_delete == hashTable[hash % tableSize]){
            printf("IN TOP %p\n\n\n\n\n\n", hashTable[hash % tableSize]->next);
            // next puede no ser null y estar free'd
            hashTable[hash % tableSize] = hashTable[hash % tableSize]->next;
            // hashTable[hash % tableSize] = NULL;
        }
        
        // puedo hacer free de un elemento que sea un next y se rompe
        if(elem_to_delete->prev){
            elem_to_delete->prev->next = elem_to_delete->next;
        }
        if(elem_to_delete->next){
            elem_to_delete->next->prev = elem_to_delete->prev;
        }
        if(elem_to_delete == lastElemDelete){
            lastElemDelete = aux;
        }else{
            if(elem_to_delete->prev_delete){
                elem_to_delete->prev_delete->next_delete = elem_to_delete->next_delete->prev_delete;
            }
            if(elem_to_delete->next_delete){
                elem_to_delete->next_delete->prev_delete = elem_to_delete->prev_delete->next_delete;
            }
        }

        free(elem_to_delete->word.string);
        free(elem_to_delete->value.string);
        free(elem_to_delete);
        elem_to_delete = NULL;
        pthread_mutex_unlock(lock);
        pthread_mutex_unlock(&lastElemLock);
        return;
    }else{
        printf("NO MEMORY LEFT\n");
        pthread_mutex_unlock(&lastElemLock);
        exit(1);
        return;    
        // nos quedamos sin memoria, quit?
    }
    // pthread_mutex_unlock(&lastElemLock);
    // printf("OUT FREE MEMORY-------------------------------------------\n\n\n");
    // return;
}