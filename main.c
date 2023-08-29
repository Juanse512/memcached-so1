#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include "headers/io.h"
#include "headers/helpers.h"
#include "headers/hashing.h"
#include "headers/networking.h"
#include<signal.h>



int main(int argc, char *argv[])
{
    signal(SIGPIPE, SIG_IGN);
    init();
    // El programa necesita ser ejecutado como root para abrir los puertos restringidos
    if (setuid(0) != 0) {
        printf("Operacion no permitida: Ejecute el programa como root.\n");
        return 1;
    }
    
    lsock = mk_lsock(888);
    binlsock = mk_lsock(889);

    // Una vez abiertos reducimos los privilegios
    if (setuid(1000) != 0) {
        printf("Error al reducir privilegios, programa detenido por seguridad.\n");
        return 1;
    }
	wait_for_clients();
    return 0;
}