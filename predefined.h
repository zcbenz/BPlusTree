#ifndef PREDEFINED_H
#define PREDEFINED_H

#include <string.h>

namespace bpt {

/* predefined B+ info */
#ifndef BP_ORDER
    #define BP_ORDER 4
#endif

/* offsets */
#define OFFSET_META 0
#define OFFSET_INDEX OFFSET_META + sizeof(meta_t)
#define OFFSET_INDEX_END OFFSET_INDEX + meta.internal_node_num * sizeof(internal_node_t)
#define OFFSET_BLOCK OFFSET_INDEX + meta.index_size
#define OFFSET_END OFFSET_BLOCK + meta.leaf_node_num * sizeof(leaf_node_t)

/* key/value type */
typedef int value_t;
struct key_t {
    char k[32];

    key_t(const char *str = "")
    {
        strcpy(k, str);
    }
};

inline int keycmp(const key_t &l, const key_t &r) {
    return strcmp(l.k, r.k);
}

#define OPERATOR_KEYCMP(type) \
    bool operator< (const key_t &l, const type &r) {\
        return keycmp(l, r.key) < 0;\
    }\
    bool operator< (const type &l, const key_t &r) {\
        return keycmp(l.key, r) < 0;\
    }

}

#endif /* end of PREDEFINED_H */
