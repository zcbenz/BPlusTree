#include "../bpt.h"
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

        if (argc == 4) {
            value_t value;
            if (database.search(argv[3], &value) != 0)
                printf("Key %s not found\n", argv[3]);
            else
                printf("%d\n", value);
        } else {
            bpt::key_t start(argv[3]);
            value_t values[512];
            bool next = true;
            while (next) {
                int ret = database.search_range(
                        &start, argv[4], values, 512, &next);
                if (ret < 0)
                    break;
                for (int i = 0; i < ret; i++)
                    printf("%d\n", values[i]);
            }
        }
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
    } else {
        fprintf(stderr, "Invalid command: %s\n", argv[2]);
        return 1;
    }
    
    return 0;
}

