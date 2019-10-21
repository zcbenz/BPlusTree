#ifndef PREDEFINED_H
#define PREDEFINED_H

#include <string.h>

namespace bpt {

/* predefined B+ info */
#define BP_ORDER 4

/* key/value type */
typedef int value_t;
struct key_t {
    char k[16];

    key_t(const char *str = "")
    {
        bzero(k, sizeof(k));
        strcpy(k, str);
    }

    operator bool() const {
        return strcmp(k, "");
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
    }\
    bool operator== (const key_t &l, const type &r) {\
        return keycmp(l, r.key) == 0;\
    }\
    bool operator== (const type &l, const key_t &r) {\
        return keycmp(l.key, r) == 0;\
    }

}

#endif /* end of PREDEFINED_H */
