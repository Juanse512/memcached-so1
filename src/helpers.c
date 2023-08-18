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
#include "../headers/helpers.h"
//insert_word: (Word *, Word *) -> (Word*)
// Toma el primer elemento de una lista y un nuevo nodo e inserta este al final de la cola
Word * insert_word(Word * word, Word * newWord){
    
    set_firstElem(newWord);
    pthread_mutex_lock(&lastElemLock);
    // Si la cola de borrado esta vacia coloco el nuevo nodo como el primer elemento a borrar
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

//clean_array: (Word **, int) -> ()
// Toma un array de punteros y los pone a NULL
void clean_array(Word ** hashTable, int counter){
    for(int i = 0; i < counter; i++)
    {
        hashTable[i] = NULL;
    }
}
// init_lock: (pthread_mutex_t*, pthread_mutexattr_t*) -> ()
// Dado un lock y una struct de atributos, inicializa el lock
void init_lock(pthread_mutex_t* lock, pthread_mutexattr_t* Attr){
    if (pthread_mutex_init(lock, Attr) != 0) {
        printf("\n Fallo en la inicialización de mutex\n");
        exit(1);
    }
}

// init: () -> ()
// Inicializa todas las variables necesarias para que el programa funcione
void init(){
    struct rlimit lim;
    lim.rlim_cur = MAX_MEMORY_CUR;
    lim.rlim_max = MAX_MEMORY_MAX;

    if(setrlimit(RLIMIT_AS, &lim) == -1)
        fprintf(stderr, "%s\n", "err\n");
    
    long long free_ram = sysconf(_SC_AVPHYS_PAGES) * sysconf(_SC_PAGESIZE);

    if(free_ram > MAX_FREE_RAM)
        free_ram = MAX_FREE_RAM;
    

    tableSize = free_ram / (sizeof(Word *) / 4);
    PUTS = 0;
    GETS = 0;
    DELS = 0;
    KEYVALUES = 0; 
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
    locks = robust_malloc((sizeof(pthread_mutex_t) * lockSize) + 1, 1);
    for(int i = 0; i < lockSize; i++){
        init_lock(&locks[i], &Attr);
    }
    hashTable = robust_malloc(sizeof(Word *) * tableSize, 1);
    clean_array(hashTable, tableSize);
    return;
}

//parser: (char*) -> (char **)
// Dado un string, lo separa en espacios y devuelve un nuevo array de arrays con estos valores
char ** parser(char* str){

    char ** tP = robust_malloc(sizeof(char*)*4 , 0);
    if(tP == NULL)
        return NULL;
    
    char *token; 
    
    token = strtok(str, " ");
    tP[0] = robust_malloc(sizeof(char) * strlen(token), 0); 
    if(tP[0] == NULL)
        return NULL;
    
    strcpy(tP[0], token);
    
    int i = 1;
    token = strtok(NULL, " ");
    
    while(token != NULL){
        tP[i] = robust_malloc(sizeof(char) * strlen(token), 0); 
        if(tP[i] == NULL)
            return NULL;
    
        strcpy(tP[i++], token);
        token = strtok(NULL, " ");
    }

    for(; i < 4; i++)
        tP[i] = NULL;
    
    return tP;
}

//get_lock: (unsigned int) -> (pthread_mutex_t*)
// Dada una posicion, devuelve el lock que le corresponde
pthread_mutex_t* get_lock(unsigned int position){
    unsigned int pos = (position % lockSize);
    pthread_mutex_t * lock = &locks[pos];
    return lock;
}

//delete_element: (Word*, unsigned int) -> ()
// Dado un elemento y su hash borra el elemento y conecta los nodos relacionados
void delete_element(Word * actual, unsigned int hash){
    
    // Si el nodo es el primer elemento de la lista, actualiza el puntero de esta
    if(hashTable[hash % tableSize] == actual)
        hashTable[hash % tableSize] = actual->next;

    Word * next_delete = actual->next_delete;
    Word * prev_delete = actual->prev_delete;
    pthread_mutex_t* prevL = NULL;
    pthread_mutex_t* nextL = NULL;
    // Actualizamos los valores de la cola de borrado
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
    // Si el elemento es el ultimo de la cola de borrado, actualizamos esta con el siguiente nodo a borrar
    if(actual == lastElemDelete)
        lastElemDelete = prev_delete;
    
    pthread_mutex_unlock(&lastElemLock);

    pthread_mutex_lock(&firstElemLock);
    // Si el elemento es el primero de la cola de borrado, actualizamos esta con el siguiente nodo
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

//compare_string: (CompString, CompString) -> (int)
// Nueva funcion de comparacion para elementos del modo binario, a prueba de \n
// Devuelve 0 si son iguales, 1 si no
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

//free_memory: () -> (int)
// Se encarga de eliminar un elemento de la cola de borrado cuando no hay memoria disponible
// Devuelve 1 si elimino correctamente, -1 si no
int free_memory(){
    pthread_mutex_lock(&lastElemLock);
    
    // Si no hay elementos en la cola no hay mas memoria
    if(lastElemDelete == NULL){
        pthread_mutex_unlock(&lastElemLock);
        return -1;
    }

    Word * aux = lastElemDelete->prev_delete;
    Word * elem_to_delete = lastElemDelete;
    unsigned int hash = hash_first(elem_to_delete->word.string);
    pthread_mutex_t* lock = get_lock(hash % tableSize);
    
    // Trata de obtener el lock del elemento a borrar, si esta en uso, se mueve al siguiente elemento de la cola
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

    // Si no hay elemento para borrar devolvemos -1
    if(elem_to_delete == NULL){
        pthread_mutex_unlock(&lastElemLock);
        return -1;
    }

    // Si el elemento a borrar es el primero de su lista enlazada, actualiza el puntero de esta
    if(elem_to_delete == hashTable[hash % tableSize])
        hashTable[hash % tableSize] = hashTable[hash % tableSize]->next;
    


    // Actualiza los valores de sus vecinos
    if(elem_to_delete->prev)
        elem_to_delete->prev->next = elem_to_delete->next;
    
    if(elem_to_delete->next)
        elem_to_delete->next->prev = elem_to_delete->prev;

    if(elem_to_delete->prev_delete)
        elem_to_delete->prev_delete->next_delete = elem_to_delete->next_delete;
        
    if(elem_to_delete->next_delete)
        elem_to_delete->next_delete->prev_delete = elem_to_delete->prev_delete;


    if(elem_to_delete == lastElemDelete)
        lastElemDelete = aux;


    free(elem_to_delete->word.string);
    free(elem_to_delete->value.string);
    free(elem_to_delete);

    pthread_mutex_unlock(lock);
    pthread_mutex_unlock(&lastElemLock);

    // Restamos a la cantidad de key-values
    pthread_mutex_lock(&kvLock);
    KEYVALUES--;
    pthread_mutex_unlock(&kvLock);

    return 1;
}


//robust_malloc(size_t, int) -> void *
// Dado un tamaño y una flag de init, hace un malloc que solo devuelve NULL si no tiene memoria y no puede liberar mas
// La flag de init indica si ese malloc es parte de la inicializacion del programa, si lo es y no hay memoria el programa se detiene
// ya que no tiene memoria suficiente para inciar, si no devuelve NULL y es handleado por la funcion que lo llama
void * robust_malloc(size_t size, int init){
    void * pointer = malloc(size);
    int res = 1;
    while(pointer == NULL){
        res = free_memory();
        if(res == -1)
            break;
        pointer = malloc(size);
    }
    if(pointer == NULL && init){
        printf("Memoria insuficiente para iniciar el servidor\n");
        exit(1);
    }
    return pointer;
}

//set_firstElem: (Word *) -> ()
// Toma un nodo y lo coloca como ultimo elemento a borrar en la cola de borrado
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