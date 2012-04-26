#ifndef BPT_H
#define BPT_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <memory>
using std::unique_ptr;

namespace bpt {

typedef int value_t;
struct key_t {
    char k[32];

    key_t()
    {}

    key_t(const char *str)
    {
        strcpy(k, str);
    }
};

inline int keycmp(const key_t &l, const key_t &r) {
    return strcmp(l.k, r.k);
}

/* predefined B+ info */
#define BP_ORDER 4

/* meta information of B+ tree */
typedef struct {
    size_t order; /* `order` of B+ tree */
    size_t index_size; /* size of the whole index area */
    size_t value_size; /* size of value */
    size_t key_size;   /* size of key */
    size_t internal_node_num; /* how many internal nodes */
    size_t leaf_node_num;     /* how many leafs */
    size_t height;            /* height of tree (exclude leafs) */
} meta_t;

/* internal nodes' index segment */
typedef struct {
    key_t key;
    off_t child; /* child's offset according to the internal node */
} index_t;

/***
 * internal node block
 * | size_t n | key_t key, size_t child | key_t key, size_t child | ... |
 ***/
typedef struct {
    size_t n; /* how many children */
    index_t children[BP_ORDER];
} internal_node_t;

/* the final record of value */
struct record_t {
    key_t key;
    value_t value;
};

/***
 * leaf node block
 * | int prev | int next | size_t n | size_t i, key_t key, value_t value | ... |
 ***/
struct leaf_node_t {
    int prev;
    int next;
    size_t n;
    record_t children[BP_ORDER - 1];
};

/* the encapulated B+ tree */
class bplus_tree {
public:
    bplus_tree(const char *path, bool force_empty = false);

    /* abstract operations */
    value_t search(const key_t& key) const;
    value_t erase(const key_t& key);
    value_t insert(const key_t& key, value_t value);
    value_t update(const key_t& key, value_t value);

public:
    char path[512];

    meta_t meta;

    /* init empty tree */
    void init_from_empty();

    /* find leaf */
    off_t search_leaf_offset(const key_t &key) const;
    int search_leaf(const key_t &key, leaf_node_t *leaf) const
    {
        return map_block(leaf, search_leaf_offset(key));
    }

    /* multi-level file open/close */
    mutable FILE *fp;
    mutable int fp_level;
    inline void open_file() const;
    inline void close_file() const;

    /* read tree from disk */
    void sync_meta();
    int map_index(internal_node_t *node, off_t offset) const;
    int map_block(leaf_node_t *leaf, off_t offset) const;

    /* write tree to disk */
    void write_meta_to_disk() const;
    void write_leaf_to_disk(leaf_node_t *leaf, off_t offset);
    void write_new_leaf_to_disk(leaf_node_t *leaf);
    void sync_leaf_to_disk(leaf_node_t *leaf) const;
};

}

#endif /* end of BPT_H */
