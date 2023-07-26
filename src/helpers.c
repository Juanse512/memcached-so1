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

Word * insert_word(Word * word, unsigned int hash, char * wordChar, char * value, int lengthWord, int lengthValue, int mode){
    Word * newWord = malloc(sizeof(Word));

    while(newWord == NULL){
        freeMemory();
        newWord = malloc(sizeof(Word));
    }
    newWord->next = NULL;
    newWord->hash = hash;
    // newWord->word.string = malloc(sizeof(char)*lengthWord);
    // memcpy(newWord->word.string, wordChar, lengthWord);
    newWord->word.string = wordChar;
    newWord->word.len = lengthWord;
    // newWord->value.string = malloc(sizeof(char)*lengthValue);
    // memcpy(newWord->value.string, value, lengthValue);
    newWord->value.string = value;
    newWord->value.len = lengthValue;
    newWord->bin = mode;
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
        while(aux != NULL && aux->next){
            aux = aux->next;
        }
        aux->next = newWord;
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
    long long free_ram = sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGESIZE);
    if(free_ram > 80000){
        free_ram = 80000;
    }
    tableSize = free_ram / (sizeof(Word *) / 4);
    PUTS = 0;
    GETS = 0;
    DELS = 0;
    KEYVALUES = 0; 
    if (pthread_mutex_init(&putsLock, NULL) != 0) {
        printf("\n mutex init failed\n");
        return;
    }
    if (pthread_mutex_init(&getsLock, NULL) != 0) {
        printf("\n mutex init failed\n");
        return;
    }
    if (pthread_mutex_init(&delsLock, NULL) != 0) {
        printf("\n mutex init failed\n");
        return;
    }
    if (pthread_mutex_init(&kvLock, NULL) != 0) {
        printf("\n mutex init failed\n");
        return;
    }
    if (pthread_mutex_init(&firstElemLock, NULL) != 0) {
        printf("\n mutex init failed\n");
        return;
    }
    if (pthread_mutex_init(&lastElemLock, NULL) != 0) {
        printf("\n mutex init failed\n");
        return;
    }
    firstElemDelete = NULL;
    lastElemDelete = NULL;
    // tableSize = 100;
    lockSize = tableSize / 10;
    locks = malloc((sizeof(pthread_mutex_t) * lockSize) + 1);
    for(int i = 0; i < lockSize; i++){
        if (pthread_mutex_init(&locks[i], NULL) != 0) {
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
    printf("%d\n", i);
    return tP;
}


pthread_mutex_t* get_lock(int position){
    int pos = (position % lockSize);
    printf("position: %d %d\n", position, pos);
    pthread_mutex_t * lock = &locks[pos];
    printf("AFTER GET LOCK\n");
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
        free(lastElemDelete->word.string);
        free(lastElemDelete->value.string);
        free(lastElemDelete);
        lastElemDelete = aux;
    }else{
        // nos quedamos sin memoria, quit?
    }
    pthread_mutex_unlock(&lastElemLock);
    return;
}