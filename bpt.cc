#include "bpt.h"

#include <stdlib.h>
#include <assert.h>

#include <algorithm>
using std::binary_search;
using std::lower_bound;
using std::upper_bound;

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
        map(&meta);
}

int bplus_tree::search(const key_t& key, value_t *value) const
{
    leaf_node_t leaf;
    map(&leaf, search_leaf(key));

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
    off_t offset = search_leaf(key);
    map(&leaf, offset);

    // check if we have the same key
    if (binary_search(leaf.children, leaf.children + leaf.n, key))
        return 1;

    if (leaf.n == meta.order) {
        // split when full

        // new sibling leaf
        leaf_node_t new_leaf;
        leaf.next = alloc(&new_leaf);

        // find even split point
        size_t point = leaf.n / 2;
        if (keycmp(key, leaf.children[point].key) > 0)
            ++point;

        // split
        std::copy(leaf.children + point, leaf.children + leaf.n,
                  new_leaf.children);
        new_leaf.n = leaf.n - point;
        leaf.n = point;

        // which part do we put the key
        if (keycmp(key, new_leaf.children[0].key) < 0)
            insert_leaf_no_split(&leaf, key, value);
        else
            insert_leaf_no_split(&new_leaf, key, value);

        // insert new index key
        new_leaf.parent = leaf.parent = insert_key_to_index(
                leaf.parent, new_leaf.children[0].key, offset, leaf.next);

        // save leafs with parent updated
        unmap(&meta);
        unmap(&leaf, offset);
        unmap(&new_leaf, leaf.next);
    } else {
        insert_leaf_no_split(&leaf, key, value);
        unmap(&leaf, offset);
    }

    return 0;
}

void bplus_tree::insert_leaf_no_split(leaf_node_t *leaf,
                                      const key_t &key, const value_t &value)
{
    // insert into array when leaf is not full
    record_t *r = upper_bound(leaf->children, leaf->children + leaf->n, key);

    // move forward 1 element
    for (record_t *c = leaf->children + leaf->n; c > r; --c)
        *c = *(c - 1);

    r->key = key;
    r->value = value;
    leaf->n++;
}

int bplus_tree::insert_key_to_index(int offset, key_t key,
                                    off_t old, off_t after)
{
    assert(offset >= -1);

    if (offset == -1) {
        // create new root node
        internal_node_t root;
        meta.root_offset = alloc(&root);
        meta.height++;

        // insert `old` and `after`
        root.n = 1;
        root.children[0].key = key;
        root.children[0].child = old;
        root.children[1].child = after;

        unmap(&meta);
        unmap(&root, meta.root_offset);
        return meta.root_offset;
    }

    internal_node_t node;
    map(&node, offset);
    assert(node.n + 1 <= meta.order);

    if (node.n + 1 == meta.order) {
        // split when full

        internal_node_t new_node;
        off_t new_offset = alloc(&new_node);

        // split
        // note: there are node.n + 1 elements in node.children
        size_t point = node.n / 2 + 1;
        std::copy(node.children + point, node.children + node.n + 1,
                  new_node.children);
        new_node.n = node.n - point;
        node.n -= point + 1;

        // give the middle key to the parent
        // note: middle key's child is reserved in children[node.n]
        new_node.parent = node.parent = insert_key_to_index(
                node.parent, node.children[point].key, offset, new_offset);
        
        unmap(&meta);
        unmap(&node, offset);
        unmap(&new_node, new_offset);
    } else {
        insert_key_to_index_no_split(&node, key, after);
        unmap(&node, offset);
    }

    return offset;
}

void bplus_tree::insert_key_to_index_no_split(internal_node_t *node,
                                             const key_t &key, off_t value)
{
    index_t *i = upper_bound(node->children, node->children + node->n,
                             key);

    // note: adding key after splitting should be dealt with differently when
    //       adding to the end or not end

    if (i == node->children + node->n) {
        // add index key to end
        node->children[node->n].key = key;
        node->children[node->n + 1].child = value;
    } else {
        // move later index forward
        for (index_t *j = node->children + node->n + 1; j > i; --j)
            *j = *(j - 1);

        // and insert this key
        i->key = key;
        i->child = value;
    }

    node->n++;
}

off_t bplus_tree::search_index(const key_t &key) const
{
    off_t org = meta.root_offset;
    int height = meta.height;
    while (height > 1) {
        internal_node_t node;
        map(&node, org);

        index_t *i = upper_bound(node.children, node.children + node.n, key);
        org = i->child;
        --height;
    }

    return org;
}

off_t bplus_tree::search_leaf(const key_t &key) const
{
    internal_node_t node;
    map(&node, search_index(key));

    index_t *i = upper_bound(node.children, node.children + node.n, key);
    return i->child;
}

void bplus_tree::init_from_empty()
{
    // init default meta
    bzero(&meta, sizeof(meta_t));
    meta.order = BP_ORDER;
    meta.index_size = sizeof(internal_node_t) * 128;
    meta.value_size = sizeof(value_t);
    meta.key_size = sizeof(key_t);
    meta.height = 1;

    // init root node
    internal_node_t root;
    meta.root_offset = alloc(&root);

    // init empty leaf
    leaf_node_t leaf;
    root.children[0].child = alloc(&leaf);
    leaf.parent = meta.root_offset;

    // save
    unmap(&meta);
    unmap(&root, meta.root_offset);
    unmap(&leaf, root.children[0].child);
}

}
