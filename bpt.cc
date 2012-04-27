#include "bpt.h"
#include <stdlib.h>
#include <assert.h>

#include <algorithm>
using std::lower_bound;
using std::upper_bound;

/* offsets */
#define OFFSET_META 0
#define OFFSET_INDEX OFFSET_META + sizeof(meta_t)
#define OFFSET_BLOCK OFFSET_INDEX + meta.index_size
#define OFFSET_END OFFSET_BLOCK + meta.leaf_node_num * sizeof(leaf_node_t)

namespace bpt {

OPERATOR_KEYCMP(index_t)
OPERATOR_KEYCMP(record_t)

bplus_tree::bplus_tree(const char *p, bool force_empty)
    : fp(NULL), fp_level(0)
{
    bzero(path, sizeof(path));
    strcpy(path, p);

    if (!force_empty) {
        FILE *fp = fopen(path, "r");
        if (!fp)
            force_empty = true;
        else
            fclose(fp);
    }

    if (force_empty)
        // create empty tree if file doesn't exist
        init_from_empty();
    else
        // read tree from file
        sync_meta();
}

int bplus_tree::search(const key_t& key, value_t *value) const
{
    leaf_node_t leaf;
    map_block(&leaf, search_leaf_offset(key));

    // finding the record
    record_t *record = lower_bound(leaf.children, leaf.children + leaf.n, key);
    if (record != leaf.children + leaf.n) {
        // always return the lower bound
        *value = record->value;

        return keycmp(record->key, key);
    } else {
        return -1;
    }
}

int bplus_tree::insert(const key_t& key, value_t value)
{
    leaf_node_t leaf;

    off_t offset = search_leaf_offset(key);
    map_block(&leaf, offset);

    if (leaf.n == meta.order - 1) {
        // TODO split when full

    } else {
        // insert into array when leaf is not full
        record_t *r = upper_bound(leaf.children, leaf.children + leaf.n, key);
        if (r != leaf.children + leaf.n) {
            // same key?
            if (keycmp((r - 1)->key, key) == 0)
                return 1;

            // move forward 1 element
            std::copy(r, leaf.children + leaf.n, r + 1);
        }

        r->key = key;
        r->value = value;
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
    meta.index_size = sizeof(internal_node_t) * 128;
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
    off_t org = sizeof(meta_t);
    while (height > 0) {
        // get current node
        internal_node_t node;
        map_index(&node, org);

        // move org to correct child
        index_t *r = lower_bound(node.children, node.children + node.n, key);
        org += r->child;

        --height;
    }

    return offset;
}

void bplus_tree::open_file() const
{
    // `rb+` will make sure we can write everywhere without truncating file
    if (fp_level == 0) {
        fp = fopen(path, "rb+");
        if (!fp)
            fp = fopen(path, "wb+");
    }

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
    fseek(fp, OFFSET_META, SEEK_SET);
    fread(&meta, sizeof(meta_t), 1, fp);
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
    fseek(fp, OFFSET_META, SEEK_SET);
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

    if (leaf->next == -1) {
        // write new leaf at the end
        write_leaf_to_disk(leaf, OFFSET_END);
    } else {
        // TODO seek to the write position
    }

    // increse leaf counter
    meta.leaf_node_num++;
    write_meta_to_disk();

    close_file();
}

}
