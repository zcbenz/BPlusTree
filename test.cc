#include <assert.h>
#include <stdio.h>

#include "bpt.h"
using bpt::bplus_tree;

int main(int argc, char *argv[])
{
    {
    bplus_tree empty_tree("test.db", true);
    assert(empty_tree.meta.order == 4);
    assert(empty_tree.meta.value_size == sizeof(bpt::value_t));
    assert(empty_tree.meta.key_size == sizeof(bpt::key_t));
    assert(empty_tree.meta.internal_node_num == 0);
    assert(empty_tree.meta.leaf_node_num == 1);
    assert(empty_tree.meta.height == 0);
    printf("\033[32m%s\033[0m\n", "EmptyTree Passed");
    }

    {
    bplus_tree existing_tree("test.db");
    assert(existing_tree.meta.order == 4);
    assert(existing_tree.meta.value_size == sizeof(bpt::value_t));
    assert(existing_tree.meta.key_size == sizeof(bpt::key_t));
    assert(existing_tree.meta.internal_node_num == 0);
    assert(existing_tree.meta.leaf_node_num == 1);
    assert(existing_tree.meta.height == 0);
    printf("\033[32m%s\033[0m\n", "ExistingEmptyTree Passed");

    existing_tree.insert("t1", 1);
    existing_tree.insert("t2", 2);
    existing_tree.insert("t3", 3);
    bpt::leaf_node_t leaf;
    existing_tree.search_leaf("t1", &leaf);
    assert(leaf.n == 3);
    assert(bpt::keycmp(leaf.children[0].key, "t1") == 0);
    assert(bpt::keycmp(leaf.children[1].key, "t2") == 0);
    assert(bpt::keycmp(leaf.children[2].key, "t3") == 0);
    assert(leaf.children[0].value == 1);
    assert(leaf.children[1].value == 2);
    assert(leaf.children[2].value == 3);
    printf("\033[32m%s\033[0m\n", "ImediateInsert3Elements Passed");
    }

    {
    bplus_tree tree("test.db");
    assert(tree.meta.order == 4);
    assert(tree.meta.value_size == sizeof(bpt::value_t));
    assert(tree.meta.key_size == sizeof(bpt::key_t));
    assert(tree.meta.internal_node_num == 0);
    assert(tree.meta.leaf_node_num == 1);
    assert(tree.meta.height == 0);
    bpt::leaf_node_t leaf;
    tree.search_leaf("t1", &leaf);
    assert(leaf.n == 3);
    assert(bpt::keycmp(leaf.children[0].key, "t1") == 0);
    assert(bpt::keycmp(leaf.children[1].key, "t2") == 0);
    assert(bpt::keycmp(leaf.children[2].key, "t3") == 0);
    assert(leaf.children[0].value == 1);
    assert(leaf.children[1].value == 2);
    assert(leaf.children[2].value == 3);
    printf("\033[32m%s\033[0m\n", "LastInsert3Elements Passed");
    }

    return 0;
}
