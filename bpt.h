#ifndef BPT_H
#define BPT_H

namespace bpt {

typedef int value_t;
typedef char key_t[32];

typedef struct {
    int order;
    int block;
    int value_size;
    int key_size;
} meta_t;

extern meta_t meta;

}

#endif /* end of BPT_H */
