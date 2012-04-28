#ifndef BPT_H
#define BPT_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "predefined.h"

namespace bpt {

/* meta information of B+ tree */
typedef struct {
    size_t order; /* `order` of B+ tree */
    size_t index_size; /* size of the whole index area */
    size_t value_size; /* size of value */
    size_t key_size;   /* size of key */
    size_t internal_node_num; /* how many internal nodes */
    size_t leaf_node_num;     /* how many leafs */
    size_t height;            /* height of tree (exclude leafs) */
    size_t index_slot; /* where to store new index */
    size_t block_slot; /* where to store new block */
    off_t root_offset; /* where is the root of internal nodes */
} meta_t;

/* internal nodes' index segment */
struct index_t {
    key_t key;
    off_t child; /* child's offset */
};

/***
 * internal node block
 * | int parent | size_t n | key_t key, size_t child | ... |
 ***/
struct internal_node_t {
    int parent; /* parent node offset */
    size_t n; /* how many children */
    index_t children[BP_ORDER];
};

/* the final record of value */
struct record_t {
    key_t key;
    value_t value;
};

/* leaf node block */
struct leaf_node_t {
    int next; /* next leaf */
    size_t n;
    record_t children[BP_ORDER];
};

/* the encapulated B+ tree */
class bplus_tree {
public:
    bplus_tree(const char *path, bool force_empty = false);

    /* abstract operations */
    int search(const key_t& key, value_t *value) const;
    int erase(const key_t& key);
    int insert(const key_t& key, value_t value);
    int update(const key_t& key, value_t value);

public:
    char path[512];
    meta_t meta;

    /* init empty tree */
    void init_from_empty();

    /* find index */
    off_t search_index(const key_t &key) const;

    /* find leaf */
    off_t search_leaf(off_t index, const key_t &key) const;
    off_t search_leaf(const key_t &key) const
    {
        return search_leaf(search_index(key), key);
    }

    /* insert into leaf without split */
    void insert_leaf_no_split(leaf_node_t *leaf,
                              const key_t &key, const value_t &value);

    /* add key to the internal node */
    int insert_key_to_index(int offset, const key_t &key,
                            off_t value, off_t after, bool is_leaf);
    void insert_key_to_index_no_split(internal_node_t *node, const key_t &key,
                                      off_t value);

    void reset_index_children_parent(index_t *begin, index_t *end, int parent);

    /* multi-level file open/close */
    mutable FILE *fp;
    mutable int fp_level;
    void open_file() const
    {
        // `rb+` will make sure we can write everywhere without truncating file
        if (fp_level == 0) {
            fp = fopen(path, "rb+");
            if (!fp) // new file
                fp = fopen(path, "wb+");
        }

        ++fp_level;
    }

    void close_file() const
    {
        if (fp_level == 1)
            fclose(fp);

        --fp_level;
    }

    /* alloc from disk */
    off_t alloc(leaf_node_t *leaf)
    {
        leaf->next = -1;
        leaf->n = 0;
        ++meta.leaf_node_num;
        ++meta.block_slot;
        return sizeof(leaf_node_t) * (meta.block_slot - 1);
    }

    off_t alloc(internal_node_t *node)
    {
        node->parent = -1;
        node->n = 0;
        ++meta.internal_node_num;
        ++meta.index_slot;
        assert(meta.internal_node_num < BP_MAXINDEX);
        return sizeof(internal_node_t) * (meta.index_slot - 1);
    }

    off_t unalloc(leaf_node_t *leaf);
    off_t unalloc(internal_node_t *node);

    /* read block from disk */
    template<class T>
    void rmap(T *block, off_t offset) const
    {
        open_file();
        fseek(fp, offset, SEEK_SET);
        fread(block, sizeof(T), 1, fp);
        close_file();
    }

    /* write block to disk */
    template<class T>
    void runmap(T *block, off_t offset) const
    {
        open_file();
        fseek(fp, offset, SEEK_SET);
        fwrite(block, sizeof(T), 1, fp);
        close_file();
    }

    void map(leaf_node_t *leaf, off_t offset) const
    {
        rmap(leaf, offset + OFFSET_BLOCK);
    }

    void map(internal_node_t *node, off_t offset) const
    {
        rmap(node, offset + OFFSET_INDEX);
    }

    void map(meta_t *m) const
    {
        rmap(m, OFFSET_META);
    }

    void unmap(leaf_node_t *leaf, off_t offset) const
    {
        runmap(leaf, offset + OFFSET_BLOCK);
    }

    void unmap(internal_node_t *node, off_t offset) const
    {
        runmap(node, offset + OFFSET_INDEX);
    }

    void unmap(meta_t *m) const
    {
        runmap(m, OFFSET_META);
    }
};

}

#endif /* end of BPT_H */
