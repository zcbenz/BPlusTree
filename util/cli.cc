#include "bpt.h"
using namespace bpt;

#include <string.h>

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s database command\n", argv[0]);
        return 1;
    }

    bpt::bplus_tree database(argv[1]);
    if (!strcmp(argv[2], "search")) {
        if (argc < 4) {
            fprintf(stderr, "Need key.\n");
            return 1;
        }

        value_t value;
        if (database.search(argv[3], &value) != 0)
            printf("Key %s not found\n", argv[3]);
        else
            printf("%d\n", value);
    } else if (!strcmp(argv[2], "insert")) {
        if (argc < 5) {
            fprintf(stderr, "Format is [insert key value]\n");
            return 1;
        }

        if (database.insert(argv[3], atoi(argv[4])) != 0)
            printf("Key %s already exists\n", argv[3]);
    } else if (!strcmp(argv[2], "update")) {
        if (argc < 5) {
            fprintf(stderr, "Format is [update key value]\n");
            return 1;
        }

        if (database.update(argv[3], atoi(argv[4])) != 0)
            printf("Key %s does not exists.\n", argv[3]);
    }
    
    return 0;
}

