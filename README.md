# Memcached
### Trabajo Práctico Final Sistemas Operativos I 
## Juan Segundo Valero

## Instrucciones:
Para compilar el programa usar
``` bash
make
```
Para correr el programa:
``` bash
sudo ./server
```
El servidor necesita permisos root para poder abrir los puertos restringidos 888 y 889, luego de abrirlos se reducen los privilegios automáticamente.
Para acceder al servidor desde un cliente en modo texto se puede utilizar:
``` bash
nc localhost 888
```
Para acceder al modo binario se puede utilizar el puerto 889 o el cliente de Erlang.
## Instrucciones cliente Erlang:
La libreria se carga con:
``` erlang
c(cliente).
```
Para abrir una conexión con el servidor se usa la funcion start:
``` erlang
Sock = start(Hostname, Port)
```
Esta función devuelve un socket que se utiliza para interactuar con el resto de las funciones.
Hostname y Port por default son "localhost" y 889 respectivamente.
Funciones para interactuar con el server:
``` erlang
get(Sock,Key).
del(Sock,Key).
stats(Sock).
put(Sock,Key,Value).
```
## Funcionamiento del programa
### Almacenamiento de los datos
Para almacenar los datos se utiliza una tabla hash, el funcionamiento de esta se basa en un arreglo de listas enlazadas,
la estructura "Word" (Detallada en helpers.h) representa un nodo de una lista enlazada de la tabla hash. Para localizar o ingresar un elemento en la tabla lo que se hace es obtener un hash utilizando el algoritmo de MurmurHash2 (https://sites.google.com/site/murmurhash/) sobre la clave, luego se obtiene la posición en el arreglo haciendo hash % tamaño_de_la_tabla y se recorre la lista de esa posición hasta encontrar (o no) el elemento deseado.

Para solucionar el problema de condición de carrera en multithreading se implementó un sistema de locks sobre la tabla hash, se crea un array de locks que tiene 1/10 del tamaño que el arreglo de la tabla hash, por lo cual hay uno cada 10 listas enlazadas, el lock que le corresponde a la lista enlazada es obtenido haciendo posición % tamaño_arreglo_locks.
### Manejo de memoria
Para solucionar el problema del desalojo cuando la memoria esta llena se utiliza un protocolo de LRU (least recently used), la implementación esta dada por una lista enlazada anidada dentro de la tabla hash que contiene a los datos. Se guarda un puntero a la cabeza y a la cola de esta lista y cuando se necesita borrar un elemento se accede al último elemento, si este está siendo utilizada por otro thread se intenta de nuevo con el siguiente elemento de la lista hasta liberar alguno, en el caso de no poder liberar mas memoria se devuelve un error al cliente.
Nota: para indicar que no hay más memoria disponible para liberar se creó un nuevo codigo EMEM (116)
