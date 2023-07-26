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
    // char key[20], value[20];

    // scanf("%s", key);
    // scanf("%s", value);

    // hash_word(key, value, 100);

    // char key2[20], value2[20];

    // scanf("%s", key2);
    // scanf("%s", value2);
    // hash_word(key2, value2, 100);
    // Word * res = find_word(key, 100);

    // printf("word: %s %s %s\n", res->value, firstElemDelete->value, firstElemDelete->next_delete->value);
    int lsock;
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
	wait_for_clients(lsock);
    return 0;
}