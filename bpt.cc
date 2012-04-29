#include "bpt.h"

#include <stdlib.h>

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

    if (!force_empty)
        // read tree from file
        if (map(&meta, OFFSET_META) != 0)
            force_empty = true;

    if (force_empty) {
        open_file("w+"); // truncate file

        // create empty tree if file doesn't exist
        init_from_empty();
        close_file();
    }
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

int bplus_tree::search_range(key_t *left, const key_t &right,
                             value_t *values, size_t max, bool *next) const
{
    if (left == NULL || keycmp(*left, right) > 0)
        return -1;

    off_t off_left = search_leaf(*left);
    off_t off_right = search_leaf(right);
    off_t off = off_left;
    size_t i = 0;
    record_t *b, *e;

    leaf_node_t leaf;
    while (off != off_right && off != 0 && i < max) {
        map(&leaf, off);

        // start point
        if (off_left == off) 
            b = lower_bound(leaf.children, leaf.children + leaf.n, *left);
        else
            b = leaf.children;

        // copy
        e = leaf.children + leaf.n;
        for (; b != e && i < max; ++b, ++i)
            values[i] = b->value;

        off = leaf.next;
    }

    // the last leaf
    if (i < max) {
        map(&leaf, off_right);

        b = lower_bound(leaf.children, leaf.children + leaf.n, *left);
        e = upper_bound(leaf.children, leaf.children + leaf.n, right);
        for (; b != e && i < max; ++b, ++i)
            values[i] = b->value;
    }

    if (next != NULL) {
        if (i == max && b != e) {
            *next = true;
            *left = b->key;
        } else {
            *next = false;
        }
    }

    return i;
}

int bplus_tree::insert(const key_t& key, value_t value)
{
    off_t parent = search_index(key);
    off_t offset = search_leaf(parent, key);
    leaf_node_t leaf;
    map(&leaf, offset);

    // check if we have the same key
    if (binary_search(leaf.children, leaf.children + leaf.n, key))
        return 1;

    if (leaf.n == meta.order) {
        // split when full

        // new sibling leaf
        leaf_node_t new_leaf;
        new_leaf.next = leaf.next;
        leaf.next = alloc(&new_leaf);

        // find even split point
        size_t point = leaf.n / 2;
        bool place_right = keycmp(key, leaf.children[point].key) > 0;
        if (place_right)
            ++point;

        // split
        std::copy(leaf.children + point, leaf.children + leaf.n,
                  new_leaf.children);
        new_leaf.n = leaf.n - point;
        leaf.n = point;

        // which part do we put the key
        if (place_right)
            insert_leaf_no_split(&new_leaf, key, value);
        else
            insert_leaf_no_split(&leaf, key, value);

        // save leafs
        unmap(&meta, OFFSET_META);
        unmap(&leaf, offset);
        unmap(&new_leaf, leaf.next);

        // insert new index key
        insert_key_to_index(parent, new_leaf.children[0].key,
                            offset, leaf.next, true);
    } else {
        insert_leaf_no_split(&leaf, key, value);
        unmap(&leaf, offset);
    }

    return 0;
}

int bplus_tree::update(const key_t& key, value_t value)
{
    off_t offset = search_leaf(key);
    leaf_node_t leaf;
    map(&leaf, offset);

    record_t *record = lower_bound(leaf.children, leaf.children + leaf.n, key);
    if (record != leaf.children + leaf.n)
        if (keycmp(key, record->key) == 0) {
            record->value = value;
            unmap(&leaf, offset);

            return 0;
        } else {
            return 1;
        }
    else
        return -1;
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

void bplus_tree::insert_key_to_index(off_t offset, const key_t &key,
                                     off_t old, off_t after, bool is_leaf)
{
    if (offset == 0) {
        // create new root node
        internal_node_t root;
        meta.root_offset = alloc(&root);
        meta.height++;

        // insert `old` and `after`
        root.n = 1;
        root.children[0].key = key;
        root.children[0].child = old;
        root.children[1].child = after;

        unmap(&meta, OFFSET_META);
        unmap(&root, meta.root_offset);

        // update children's parent
        if (!is_leaf)
            reset_index_children_parent(root.children,
                                        root.children + root.n + 1,
                                        meta.root_offset);
        return;
    }

    internal_node_t node;
    map(&node, offset);
    assert(node.n + 1 <= meta.order);

    if (node.n + 1 == meta.order) {
        // split when full

        internal_node_t new_node;
        off_t new_offset = alloc(&new_node);

        // find even split point
        size_t point = node.n / 2;
        bool place_right = keycmp(key, node.children[point].key) > 0;
        if (place_right)
            ++point;

        // prevent the `key` being the right `middle_key`
        // example: insert 48 into |42|45| 6|  |
        if (place_right && keycmp(key, node.children[point].key) < 0)
            point--;

        key_t middle_key = node.children[point].key;

        // split
        // note: there are node.n + 1 elements in node.children
        std::copy(node.children + point + 1, node.children + node.n + 1,
                  new_node.children);
        new_node.n = node.n - point - 1;
        new_node.parent = node.parent;
        node.n = point;

        // put the new key
        if (place_right)
            insert_key_to_index_no_split(&new_node, key, after);
        else
            insert_key_to_index_no_split(&node, key, after);

        unmap(&meta, OFFSET_META);
        unmap(&node, offset);
        unmap(&new_node, new_offset);

        // update children's parent
        if (!is_leaf)
            reset_index_children_parent(new_node.children,
                                        new_node.children + new_node.n + 1,
                                        new_offset);

        // give the middle key to the parent
        // note: middle key's child is reserved in children[node.n]
        insert_key_to_index(node.parent, middle_key, offset, new_offset, false);
    } else {
        insert_key_to_index_no_split(&node, key, after);
        unmap(&node, offset);
    }
}

void bplus_tree::insert_key_to_index_no_split(internal_node_t *node,
                                              const key_t &key, off_t value)
{
    index_t *i = upper_bound(node->children, node->children + node->n,
                             key);

    // move later index forward
    for (index_t *j = node->children + node->n + 1; j > i; --j)
        *j = *(j - 1);

    // insert this key
    i->key = key;
    i->child = (i + 1)->child;
    (i + 1)->child = value;

    node->n++;
}

void bplus_tree::reset_index_children_parent(index_t *begin, index_t *end,
                                             off_t parent)
{
    internal_node_t node;
    while (begin != end) {
        map(&node, begin->child);
        node.parent = parent;
        unmap(&node, begin->child);
        ++begin;
    }
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

off_t bplus_tree::search_leaf(off_t index, const key_t &key) const
{
    internal_node_t node;
    map(&node, index);

    index_t *i = upper_bound(node.children, node.children + node.n, key);
    return i->child;
}

void bplus_tree::init_from_empty()
{
    // init default meta
    bzero(&meta, sizeof(meta_t));
    meta.order = BP_ORDER;
    meta.value_size = sizeof(value_t);
    meta.key_size = sizeof(key_t);
    meta.height = 1;
    meta.slot = OFFSET_BLOCK;

    // init root node
    internal_node_t root;
    meta.root_offset = alloc(&root);

    // init empty leaf
    leaf_node_t leaf;
    leaf.next = 0;
    meta.leaf_offset = root.children[0].child = alloc(&leaf);

    // save
    unmap(&meta, OFFSET_META);
    unmap(&root, meta.root_offset);
    unmap(&leaf, root.children[0].child);
}

}
