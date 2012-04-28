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
            fprintf(stderr, "Need key\n");
            return 1;
        }

        value_t value;
        if (database.search(argv[3], &value) != 0)
            printf("Key %s not found\n", argv[3]);
        else
            printf("%d\n", value);
    }
    
    return 0;
}

