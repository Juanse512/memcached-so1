all: server 
server: main.c ./src/hashing.c ./src/helpers.c ./src/io.c ./src/networking.c
	- gcc main.c ./src/hashing.c ./src/helpers.c ./src/io.c ./src/networking.c -o server -pthread 
clean: server
	- rm server