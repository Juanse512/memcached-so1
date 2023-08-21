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