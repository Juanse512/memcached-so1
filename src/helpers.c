#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>
#include "../headers/io.h"

char * parse_word(char * word){
    char * parsedWord = malloc(sizeof(char) * strlen(word) + 2);
    int counter = 0; 
    for(int i = 0; i < strlen(word); i++){
        if(isalpha(word[i])){
            parsedWord[counter++] = tolower(word[i]);
        }
    }
    parsedWord[counter] = '\0';
    return parsedWord;
}

void free_accepted(Word ** acceptedWords){   
    for(int i = 0; i < 6; i++){
        if(acceptedWords[i] != NULL){
            free(acceptedWords[i]->word);
            free(acceptedWords[i]);
        }
    }
    free(acceptedWords);
}

void free_all(char * dictionary[], Word ** hashTable, int tableSize, int dicSize){   
    for(int i = 0; i < dicSize; i++){
        free(dictionary[i]);
    }
    free(dictionary);

    for(int i = 0; i < tableSize; i++){
        free_list(hashTable[i]);
    }
    free(hashTable);
}

void free_list(Word * word){
    if(word == NULL)
        return;
    if(word->word != NULL) free(word->word);
    Word * next = word->next;
    free(word);
    free_list(next);
}

char *** check_len(char ** array[], int counter, int * arraySize){
    int newSize = *arraySize;
    char ** arrayN = *array;
    if(*arraySize <= counter){
        newSize = (newSize * 3);
        arrayN = realloc(arrayN, sizeof(char *) * (newSize));
    }
    *array = arrayN;
    *arraySize = newSize;
    return array;
}

// void save_word(char * word, char * dictionary[], int index){
//     dictionary[index] = malloc(sizeof(char) * (strlen(word) + 1));
//     strcpy(dictionary[index], word);
// }

Word * insert_word(Word * word, unsigned int hash, char * wordChar, char * value){
    Word * newWord = malloc(sizeof(Word));

    while(newWord == NULL){
        if(lastElemDelete){
            Word * aux = lastElemDelete->prev_delete;
            free(lastElemDelete->word);
            free(lastElemDelete->value);
            free(lastElemDelete);
            lastElemDelete = aux;
        }else{
            // nos quedamos sin memoria, quit?
        }
    }

    newWord->next = NULL;
    newWord->hash = hash;
    newWord->word = malloc(sizeof(char)*strlen(wordChar));
    strcpy(newWord->word, wordChar);
    newWord->value = malloc(sizeof(char)*strlen(value));
    strcpy(newWord->value, value);

    if(firstElemDelete){
        firstElemDelete->prev_delete = newWord;
    }

    newWord->next_delete = firstElemDelete;

    newWord->prev_delete = NULL;
    firstElemDelete = newWord;

    if(lastElemDelete == NULL){
        lastElemDelete = newWord;
    }


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
      // ver tamaño de hashTable, asignar memoria dependiendo de la memoria disponible
    // https://stackoverflow.com/questions/14386856/c-check-currently-available-free-ram
    //TEMP FIX:
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
    tableSize = 100;
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


void parser(char* str, char tok[3][1000]){
    // const char s[2] = " ";
    
    char *token; 
    token = strtok(str, " ");
    strcpy(tok[0], token);
    int i = 1;
    token = strtok(NULL, " ");
    while(token != NULL){
        strcpy(tok[i++], token);
        token = strtok(NULL, " ");
    }
    printf("%d\n", i);
    return;
}


pthread_mutex_t* get_lock(int position){
    int pos = (position % lockSize);
    printf("position: %d %d\n", position, pos);
    return &locks[pos];
}


void delete_element(Word * prev, Word * actual){
    if(actual == lastElemDelete){
        lastElemDelete = actual->prev_delete;
        
    }
    if(actual == firstElemDelete){
        firstElemDelete = actual->next_delete;
    }
    prev->next = actual->next;
    free(actual->word);
    free(actual->value);
    free(actual);
    return;
}