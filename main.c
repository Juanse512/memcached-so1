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


int main(int argc, char *argv[])
{

    init();
    if (setuid(0) != 0) {
        perror("setuid");
        return 1;
    }
        lsock = mk_lsock(888);
        binlsock = mk_lsock(889);
    if (setuid(1000) != 0) {
        perror("setuid");
        return 1;
    }
    printf("UID: %d\n", getuid());
    printf("UID: %d\n", geteuid());
	wait_for_clients();
    return 0;
}