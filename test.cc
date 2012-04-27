#include <assert.h>
#include <stdio.h>

#define PRINT(a) printf("\033[33m%s\033[0m \033[32m%s\033[0m\n", a, "Passed")

#define BP_ORDER 4
#include "bpt.h"
using bpt::bplus_tree;

int main(int argc, char *argv[])
{
    {
    bplus_tree tree("test.db", true);
    assert(tree.meta.order == 4);
    assert(tree.meta.value_size == sizeof(bpt::value_t));
    assert(tree.meta.key_size == sizeof(bpt::key_t));
    assert(tree.meta.internal_node_num == 1);
    assert(tree.meta.leaf_node_num == 1);
    assert(tree.meta.height == 1);
    assert(tree.meta.block_slot == 1);
    assert(tree.meta.index_slot == 1);
    PRINT("EmptyTree");
    }

    {
    bplus_tree tree("test.db");
    assert(tree.meta.order == 4);
    assert(tree.meta.value_size == sizeof(bpt::value_t));
    assert(tree.meta.key_size == sizeof(bpt::key_t));
    assert(tree.meta.internal_node_num == 1);
    assert(tree.meta.leaf_node_num == 1);
    assert(tree.meta.height == 1);
    assert(tree.meta.block_slot == 1);
    assert(tree.meta.index_slot == 1);
    PRINT("ReReadEmptyTree");

    assert(tree.insert("t2", 2) == 0);
    assert(tree.insert("t4", 4) == 0);
    assert(tree.insert("t1", 1) == 0);
    assert(tree.insert("t3", 3) == 0);
    }

    {
    bplus_tree tree("test.db");
    assert(tree.meta.order == 4);
    assert(tree.meta.value_size == sizeof(bpt::value_t));
    assert(tree.meta.key_size == sizeof(bpt::key_t));
    assert(tree.meta.internal_node_num == 1);
    assert(tree.meta.leaf_node_num == 1);
    assert(tree.meta.height == 1);
    bpt::leaf_node_t leaf;
    tree.map(&leaf, tree.search_leaf("t1"));
    assert(leaf.n == 4);
    assert(bpt::keycmp(leaf.children[0].key, "t1") == 0);
    assert(bpt::keycmp(leaf.children[1].key, "t2") == 0);
    assert(bpt::keycmp(leaf.children[2].key, "t3") == 0);
    assert(bpt::keycmp(leaf.children[3].key, "t4") == 0);
    bpt::value_t value;
    assert(tree.search("t1", &value) == 0);
    assert(value == 1);
    assert(tree.search("t2", &value) == 0);
    assert(value == 2);
    assert(tree.search("t3", &value) == 0);
    assert(value == 3);
    assert(tree.search("t4", &value) == 0);
    assert(value == 4);
    assert(tree.insert("t1", 4) == 1);
    assert(tree.insert("t2", 4) == 1);
    assert(tree.insert("t3", 4) == 1);
    assert(tree.insert("t4", 4) == 1);
    PRINT("Insert4Elements");

    assert(tree.insert("t5", 5) == 0);
    }

    {
    bplus_tree tree("test.db");
    assert(tree.meta.order == 4);
    assert(tree.meta.value_size == sizeof(bpt::value_t));
    assert(tree.meta.key_size == sizeof(bpt::key_t));
    assert(tree.meta.internal_node_num == 1);
    assert(tree.meta.leaf_node_num == 2);
    assert(tree.meta.height == 1);

    bpt::internal_node_t index;
    off_t index_off = tree.search_index("t1");
    tree.map(&index, index_off);
    assert(index.n == 1);
    assert(index.parent == -1);
    assert(bpt::keycmp(index.children[0].key, "t4") == 0);

    bpt::leaf_node_t leaf1, leaf2;
    off_t leaf1_off = tree.search_leaf("t1");
    assert(leaf1_off == index.children[0].child);
    tree.map(&leaf1, leaf1_off);
    assert(leaf1.parent == index_off);
    assert(leaf1.n == 3);
    assert(bpt::keycmp(leaf1.children[0].key, "t1") == 0);
    assert(bpt::keycmp(leaf1.children[1].key, "t2") == 0);
    assert(bpt::keycmp(leaf1.children[2].key, "t3") == 0);

    off_t leaf2_off = tree.search_leaf("t4");
    assert(leaf1.next == leaf2_off);
    assert(leaf2_off == index.children[1].child);
    tree.map(&leaf2, leaf2_off);
    assert(leaf2.parent == index_off);
    assert(leaf2.n == 2);
    assert(bpt::keycmp(leaf2.children[0].key, "t4") == 0);
    assert(bpt::keycmp(leaf2.children[1].key, "t5") == 0);

    PRINT("SplitLeafBy2");
    }

    {
    bplus_tree tree("test.db");
    assert(tree.insert("t1", 4) == 1);
    assert(tree.insert("t2", 4) == 1);
    assert(tree.insert("t3", 4) == 1);
    assert(tree.insert("t4", 4) == 1);
    assert(tree.insert("t5", 4) == 1);
    bpt::value_t value;
    assert(tree.search("t1", &value) == 0);
    assert(value == 1);
    assert(tree.search("t2", &value) == 0);
    assert(value == 2);
    assert(tree.search("t3", &value) == 0);
    assert(value == 3);
    assert(tree.search("t4", &value) == 0);
    assert(value == 4);
    assert(tree.search("t5", &value) == 0);
    assert(value == 5);
    PRINT("Search2Leaf");
    }

    return 0;
}
