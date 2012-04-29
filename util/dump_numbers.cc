#include "../bpt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int start = 0;
    int end = 90000000;

    if (argc > 2)
        start = atoi(argv[2]);
    if (argc > 3)
        end = atoi(argv[3]);

    if (argc == 1 || start >= end) {
        fprintf(stderr, "usage: %s database [start] [end]\n", argv[0]);
        return 1;
    }

    bpt::bplus_tree database(argv[1], true);
    for (int i = start; i <= end; i++) {
        if (i % 1000 == 0)
            printf("%d\n", i);
        char key[16] = { 0 };
        sprintf(key, "%d", i);
        database.insert(key, i);
    }
    printf("%d\n", end);
    printf("done\n");
    
    return 0;
}
