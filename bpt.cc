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

int bplus_tree::remove(const key_t& key)
{
    off_t offset = search_leaf(key);
    leaf_node_t leaf;
    map(&leaf, offset);

    if (!binary_search(leaf.children, leaf.children + leaf.n, key))
        return -1;

    size_t min_n = meta.leaf_node_num == 1 ? 0 : meta.order / 2;
    assert(leaf.n >= min_n && leaf.n <= meta.order);

    if (leaf.n == min_n) {
        // merge or borrow

        // delete the key
        remove_record_no_merge(&leaf, key);

        // first borrow from left
        bool borrowed = false;
        if (leaf.prev != 0)
            borrowed = borrow_record(false, &leaf);

        // then borrow from right
        if (!borrowed && leaf.next != 0)
            borrowed = borrow_record(true, &leaf);

        // TODO finally we merge
        if (!borrowed) {
        }

        unmap(&leaf, offset);
    } else {
        // just delete key
        remove_record_no_merge(&leaf, key);
        unmap(&leaf, offset);
    }

    return 0;
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
        cat_node(offset, &leaf, &new_leaf);

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
            insert_record_no_split(&new_leaf, key, value);
        else
            insert_record_no_split(&leaf, key, value);

        // save leafs
        unmap(&meta, OFFSET_META);
        unmap(&leaf, offset);
        unmap(&new_leaf, leaf.next);

        // insert new index key
        insert_key_to_index(parent, new_leaf.children[0].key,
                            offset, leaf.next);
    } else {
        insert_record_no_split(&leaf, key, value);
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

bool bplus_tree::borrow_record(bool from_right, leaf_node_t *borrower)
{
    off_t lender_off = from_right ? borrower->next : borrower->prev;
    leaf_node_t lender;
    map(&lender, lender_off);

    assert(lender.n >= meta.order / 2);
    if (lender.n != meta.order / 2) {
        record_t *where_to_lend, *where_to_put;

        // decide offset and update parent's index key
        if (from_right) {
            where_to_lend = lender.children;
            where_to_put = borrower->children + borrower->n;
            change_parent_child(borrower->parent, borrower->children[0].key,
                                lender.children[1].key);
        } else {
            where_to_lend = lender.children + lender.n - 1;
            where_to_put = borrower->children;
            change_parent_child(lender.parent, lender.children[0].key,
                                where_to_lend->key);
        }

        insert_record_no_split(borrower, where_to_put,
                               where_to_lend->key, where_to_lend->value);
        remove_record_no_merge(&lender, where_to_lend);
        unmap(&lender, lender_off);
        return true;
    }

    return false;
}

void bplus_tree::change_parent_child(off_t parent, const key_t &o,
                                     const key_t &n)
{
    internal_node_t node;
    map(&node, parent);

    index_t *w = upper_bound(node.children, node.children + node.n, o);
    if (w != node.children + node.n) {
        w->key = n;
        unmap(&node, parent);
    }
}

void bplus_tree::remove_record_no_merge(leaf_node_t *leaf, const key_t &key)
{
    remove_record_no_merge(
            leaf, lower_bound(leaf->children, leaf->children + leaf->n, key));
}

void bplus_tree::remove_record_no_merge(leaf_node_t *leaf, record_t *to_delete)
{
    for (; to_delete != leaf->children + leaf->n - 1; ++to_delete)
        *to_delete = *(to_delete + 1);
    leaf->n--;
}

void bplus_tree::insert_record_no_split(leaf_node_t *leaf,
                                        const key_t &key, const value_t &value)
{
    insert_record_no_split(leaf,
            upper_bound(leaf->children, leaf->children + leaf->n, key),
            key, value);
}

void bplus_tree::insert_record_no_split(leaf_node_t *leaf, record_t *where,
                                        const key_t &key, const value_t &value)
{
    // move forward 1 element
    for (record_t *c = leaf->children + leaf->n; c > where; --c)
        *c = *(c - 1);

    where->key = key;
    where->value = value;
    leaf->n++;
}

void bplus_tree::insert_key_to_index(off_t offset, const key_t &key,
                                     off_t old, off_t after)
{
    if (offset == 0) {
        // create new root node
        internal_node_t root;
        root.next = root.prev = root.parent = 0;
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
        cat_node(offset, &node, &new_node);

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
        node.n = point;

        // put the new key
        if (place_right)
            insert_key_to_index_no_split(&new_node, key, after);
        else
            insert_key_to_index_no_split(&node, key, after);

        unmap(&meta, OFFSET_META);
        unmap(&node, offset);
        unmap(&new_node, node.next);

        // update children's parent
        reset_index_children_parent(new_node.children,
                                    new_node.children + new_node.n + 1,
                                    node.next);

        // give the middle key to the parent
        // note: middle key's child is reserved in children[node.n]
        insert_key_to_index(node.parent, middle_key, offset, node.next);
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
    // this function can change both internal_node_t and leaf_node_t's parent
    // field, but we should ensure that:
    // 1. sizeof(internal_node_t) <= sizeof(leaf_node_t)
    // 2. parent field is placed in the beginning and have same size
    internal_node_t node;
    while (begin != end) {
        map(&node, begin->child);
        node.parent = parent;
        unmap(&node, begin->child, sizeof(node.parent));
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

template<class T>
void bplus_tree::cat_node(off_t offset, T *node, T *next)
{
    // new sibling node
    next->parent = node->parent;
    next->next = node->next;
    next->prev = offset;
    node->next = alloc(next);
    // update next node's prev
    if (next->next != 0) {
        T old_next;
        map(&old_next, next->next);
        old_next.prev = node->next;
        unmap(&old_next, next->next);
    }
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
    root.next = root.prev = root.parent = 0;
    meta.root_offset = alloc(&root);

    // init empty leaf
    leaf_node_t leaf;
    leaf.next = leaf.prev = 0;
    leaf.parent = meta.root_offset;
    meta.leaf_offset = root.children[0].child = alloc(&leaf);

    // save
    unmap(&meta, OFFSET_META);
    unmap(&root, meta.root_offset);
    unmap(&leaf, root.children[0].child);
}

}
