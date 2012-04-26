#include "bpt.h"
#include <stdlib.h>
#include <assert.h>

namespace bpt {

bplus_tree::bplus_tree(const char *p, bool force_empty)
    : fp(NULL), fp_level(0)
{
    bzero(path, sizeof(path));
    strcpy(path, p);

    FILE *fp = fopen(path, "r+");
    fclose(fp);
    if (!fp || force_empty) {
        // create empty tree if file doesn't exist
        init_from_empty();
    } else {
        // read tree from file
        sync_meta();
    }
}

value_t bplus_tree::search(const key_t& key) const
{
    leaf_node_t leaf;
    map_block(&leaf, search_leaf_offset(key));

    // TODO binary search
    size_t i;
    int ret;
    for (i = 0; i < leaf.n; i++)
        if ((ret = keycmp(leaf.children[i].key, key)) > 0)
            break;

    if (ret == 0)
        return leaf.children[i].value;
    else
        return value_t();
}

value_t bplus_tree::insert(const key_t& key, value_t value)
{
    leaf_node_t leaf;

    off_t offset = search_leaf_offset(key);
    map_block(&leaf, offset);

    if (leaf.n == meta.order - 1) {
        // TODO split when full

    } else {
        size_t i;
        int ret = -1;
        // TODO use binary search
        for (i = 0; i < leaf.n; i++)
            if ((ret = keycmp(leaf.children[i].key, key)) > 0)
                break;

        // same key?
        if (ret == 0)
            return leaf.children[i].value;

        // move afterward
        if (i < leaf.n)
            for (size_t j = leaf.n; j > i; --j)
                leaf.children[j] = leaf.children[j - 1];

        leaf.children[i].key = key;
        leaf.children[i].value = value;
        leaf.n++;
    }

    // save
    write_leaf_to_disk(&leaf, offset);

    return value;
}

void bplus_tree::init_from_empty()
{
    // init default meta
    meta.order = BP_ORDER;
    meta.index_size = sizeof(internal_node_t) * 64;
    meta.value_size = sizeof(value_t);
    meta.key_size = sizeof(key_t);
    meta.internal_node_num = meta.leaf_node_num = meta.height = 0;
    write_meta_to_disk();

    // init empty root leaf
    leaf_node_t leaf;
    leaf.prev = leaf.next = -1;
    leaf.n = 0;
    write_new_leaf_to_disk(&leaf);
}

off_t bplus_tree::search_leaf_offset(const key_t &key) const
{
    off_t offset = sizeof(meta_t) + meta.index_size;

    // deep into the leaf
    int height = meta.height;
    while (height > 0) {
        // TODO do the search
        --height;
    }

    return offset;
}

void bplus_tree::open_file(const char *mode) const
{
    if (fp_level == 0)
        fp = fopen(path, mode);

    ++fp_level;
}

void bplus_tree::close_file() const
{
    if (fp_level == 1)
        fclose(fp);

    --fp_level;
}

void bplus_tree::sync_meta()
{
    open_file();
    fread(&meta, sizeof(meta), 1, fp);
    close_file();
}

int bplus_tree::map_index(internal_node_t *node, off_t offset) const
{
    open_file();
    fseek(fp, offset, SEEK_SET);
    fread(node, sizeof(internal_node_t), 1, fp);
    close_file();

    return 0;
}

int bplus_tree::map_block(leaf_node_t *leaf, off_t offset) const
{
    open_file();
    fseek(fp, offset, SEEK_SET);
    fread(leaf, sizeof(leaf_node_t), 1, fp);
    close_file();

    return 0;
}

void bplus_tree::write_meta_to_disk() const
{
    open_file();
    fwrite(&meta, sizeof(meta_t), 1, fp);
    close_file();
}

void bplus_tree::write_leaf_to_disk(leaf_node_t *leaf, off_t offset)
{
    open_file();
    fseek(fp, offset, SEEK_SET);
    fwrite(leaf, sizeof(leaf_node_t), 1, fp);
    close_file();
}

void bplus_tree::write_new_leaf_to_disk(leaf_node_t *leaf)
{
    open_file();

    // increse leaf counter
    meta.leaf_node_num++;
    write_meta_to_disk();

    if (leaf->next == -1) {
        // write new leaf at the end
        fseek(fp, 0, SEEK_END);
    } else {
        // TODO seek to the write position
    }

    fwrite(leaf, sizeof(leaf_node_t), 1, fp);

    close_file();
}

}
