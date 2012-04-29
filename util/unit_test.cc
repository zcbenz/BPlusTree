#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <algorithm>

#define PRINT(a) fprintf(stderr, "\033[33m%s\033[0m \033[32m%s\033[0m\n", a, "Passed")

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
    assert(index.parent == 0);
    assert(bpt::keycmp(index.children[0].key, "t4") == 0);

    bpt::leaf_node_t leaf1, leaf2;
    off_t leaf1_off = tree.search_leaf("t1");
    assert(leaf1_off == index.children[0].child);
    tree.map(&leaf1, leaf1_off);
    assert(leaf1.n == 3);
    assert(bpt::keycmp(leaf1.children[0].key, "t1") == 0);
    assert(bpt::keycmp(leaf1.children[1].key, "t2") == 0);
    assert(bpt::keycmp(leaf1.children[2].key, "t3") == 0);

    off_t leaf2_off = tree.search_leaf("t4");
    assert(leaf1.next == leaf2_off);
    assert(leaf2_off == index.children[1].child);
    tree.map(&leaf2, leaf2_off);
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

    assert(tree.insert("t6", 6) == 0);
    assert(tree.insert("t7", 7) == 0);
    assert(tree.insert("t8", 8) == 0);
    assert(tree.insert("t9", 9) == 0);
    assert(tree.insert("ta", 10) == 0);
    }

    {
    bplus_tree tree("test.db");
    assert(tree.meta.internal_node_num == 1);
    assert(tree.meta.leaf_node_num == 3);
    assert(tree.meta.height == 1);

    bpt::internal_node_t index;
    off_t index_off = tree.search_index("t8");
    tree.map(&index, index_off);
    assert(index.n == 2);
    assert(index.parent == 0);
    assert(bpt::keycmp(index.children[0].key, "t4") == 0);
    assert(bpt::keycmp(index.children[1].key, "t7") == 0);

    bpt::leaf_node_t leaf1, leaf2, leaf3;
    off_t leaf1_off = tree.search_leaf("t3");
    off_t leaf2_off = tree.search_leaf("t5");
    off_t leaf3_off = tree.search_leaf("ta");
    tree.map(&leaf1, leaf1_off);
    tree.map(&leaf2, leaf2_off);
    tree.map(&leaf3, leaf3_off);
    assert(index.children[0].child == leaf1_off);
    assert(index.children[1].child == leaf2_off);
    assert(index.children[2].child == leaf3_off);
    assert(leaf1.next == leaf2_off);
    assert(leaf2.next == leaf3_off);
    assert(leaf3.next == 0);
    PRINT("SplitLeafBy3");
    }

    {
    bplus_tree tree("test.db", true);
    assert(tree.insert("t00", 0) == 0);
    assert(tree.insert("t01", 1) == 0);
    assert(tree.insert("t02", 2) == 0);
    assert(tree.insert("t03", 3) == 0);
    assert(tree.insert("t04", 4) == 0);
    assert(tree.insert("t05", 5) == 0);
    assert(tree.insert("t06", 6) == 0);
    assert(tree.insert("t07", 7) == 0);
    assert(tree.insert("t08", 8) == 0);
    assert(tree.insert("t09", 9) == 0);
    assert(tree.insert("t10", 10) == 0);
    assert(tree.insert("t11", 11) == 0);
    assert(tree.insert("t12", 12) == 0);
    assert(tree.insert("t13", 13) == 0);
    assert(tree.insert("t14", 14) == 0);
    }

    {
    bplus_tree tree("test.db");
    assert(tree.meta.internal_node_num == 3);
    assert(tree.meta.leaf_node_num == 5);
    assert(tree.meta.height == 2);

    bpt::internal_node_t node1, node2, root;
    tree.map(&root, tree.meta.root_offset);
    off_t node1_off = tree.search_index("t03");
    off_t node2_off = tree.search_index("t14");
    tree.map(&node1, node1_off);
    tree.map(&node2, node2_off);
    assert(root.n == 1);
    assert(root.children[0].child == node1_off);
    assert(root.children[1].child == node2_off);
    assert(bpt::keycmp(root.children[0].key, "t09") == 0);
    assert(node1.n == 2);
    assert(bpt::keycmp(node1.children[0].key, "t03") == 0);
    assert(bpt::keycmp(node1.children[1].key, "t06") == 0);
    assert(node2.n == 1);
    assert(bpt::keycmp(node2.children[0].key, "t12") == 0);

    bpt::value_t value;
    for (int i = 0; i < 10; i++) {
        char key[8] = { 0 };
        sprintf(key, "t0%d", i);
        assert(tree.search(key, &value) == 0);
        assert(value == i);
    }
    for (int i = 10; i < 14; i++) {
        char key[8] = { 0 };
        sprintf(key, "t%d", i);
        assert(tree.search(key, &value) == 0);
        assert(value == i);
    }

    PRINT("CreateNewRoot");
    }

    {
    bplus_tree tree("test.db", true);
    for (int i = 0; i < 12; i++) {
        char key[8] = { 0 };
        sprintf(key, "%d", i);
        assert(tree.insert(key, i) == 0);
    }
    }

    {
    bplus_tree tree("test.db");
    assert(tree.meta.internal_node_num == 1);
    assert(tree.meta.leaf_node_num == 4);
    assert(tree.meta.height == 1);

    bpt::internal_node_t node;
    tree.map(&node, tree.meta.root_offset);
    assert(node.n == 3);
    assert(bpt::keycmp(node.children[0].key, "11") == 0);
    assert(bpt::keycmp(node.children[1].key, "3") == 0);
    assert(bpt::keycmp(node.children[2].key, "6") == 0);

    off_t off1 = tree.search_leaf("0");
    off_t off2 = tree.search_leaf("11");
    off_t off3 = tree.search_leaf("5");
    off_t off4 = tree.search_leaf("6");
    assert(node.children[0].child == off1);
    assert(node.children[1].child == off2);
    assert(node.children[2].child == off3);
    assert(node.children[3].child == off4);
    bpt::leaf_node_t node1, node2, node3, node4;
    tree.map(&node1, off1);
    tree.map(&node2, off2);
    tree.map(&node3, off3);
    tree.map(&node4, off4);
    assert(node1.n == 3);
    assert(node2.n == 2);
    assert(node3.n == 3);
    assert(node4.n == 4);

    for (int i = 0; i < 12; i++) {
        char key[8] = { 0 };
        sprintf(key, "%d", i);
        bpt::value_t value;
        assert(tree.search(key, &value) == 0);
        assert(value == i);
    }
    PRINT("SplitInTheBeginning");
    }

    {
    bplus_tree tree("test.db", true);
    for (int i = 0; i < 10; i++) {
        char key[8] = { 0 };
        sprintf(key, "%d", i);
        assert(tree.insert(key, i) == 0);
    }
    assert(tree.insert("51", 51) == 0);
    assert(tree.insert("52", 52) == 0);
    }

    {
    bplus_tree tree("test.db");
    assert(tree.meta.internal_node_num == 1);
    assert(tree.meta.leaf_node_num == 4);
    assert(tree.meta.height == 1);

    bpt::internal_node_t node;
    tree.map(&node, tree.meta.root_offset);
    assert(node.n == 3);
    assert(bpt::keycmp(node.children[0].key, "3") == 0);
    assert(bpt::keycmp(node.children[1].key, "51") == 0);
    assert(bpt::keycmp(node.children[2].key, "6") == 0);

    off_t off1 = tree.search_leaf("0");
    off_t off2 = tree.search_leaf("3");
    off_t off3 = tree.search_leaf("51");
    off_t off4 = tree.search_leaf("6");
    assert(node.children[0].child == off1);
    assert(node.children[1].child == off2);
    assert(node.children[2].child == off3);
    assert(node.children[3].child == off4);
    bpt::leaf_node_t node1, node2, node3, node4;
    tree.map(&node1, off1);
    tree.map(&node2, off2);
    tree.map(&node3, off3);
    tree.map(&node4, off4);
    assert(node1.n == 3);
    assert(node2.n == 3);
    assert(node3.n == 2);
    assert(node4.n == 4);
    for (int i = 0; i < 10; i++) {
        char key[8] = { 0 };
        sprintf(key, "%d", i);
        bpt::value_t value;
        assert(tree.search(key, &value) == 0);
        assert(value == i);
    }
    assert(tree.insert("51", 51) == 1);
    assert(tree.insert("52", 52) == 1);
    PRINT("SplitInTheMiddle");
    }

    {
    bplus_tree tree("test.db", true);
    for (int i = 0; i < 15; i++) {
        char key[8] = { 0 };
        sprintf(key, "%d", i);
        assert(tree.insert(key, i) == 0);
    }
    }

    {
    bplus_tree tree("test.db");
    assert(tree.meta.internal_node_num == 3);
    assert(tree.meta.leaf_node_num == 5);
    assert(tree.meta.height == 2);

    bpt::internal_node_t node1, node2, root;
    tree.map(&root, tree.meta.root_offset);
    off_t node1_off = tree.search_index("0");
    off_t node2_off = tree.search_index("6");
    tree.map(&node1, node1_off);
    tree.map(&node2, node2_off);
    assert(root.n == 1);
    assert(bpt::keycmp(root.children[0].key, "3") == 0);
    assert(root.children[0].child == node1_off);
    assert(root.children[1].child == node2_off);
    assert(node1.n == 2);
    assert(bpt::keycmp(node1.children[0].key, "11") == 0);
    assert(bpt::keycmp(node1.children[1].key, "14") == 0);
    assert(node2.n == 1);
    assert(bpt::keycmp(node2.children[0].key, "6") == 0);
    for (int i = 0; i < 15; i++) {
        char key[8] = { 0 };
        sprintf(key, "%d", i);
        bpt::value_t value;
        assert(tree.search(key, &value) == 0);
        assert(value == i);
    }
    PRINT("CreateNewRootInMiddle");
    }

    {
    bplus_tree tree("test.db", true);
    for (int i = 0; i < 30; i++) {
        char key[8] = { 0 };
        sprintf(key, "%d", i);
        assert(tree.insert(key, i) == 0);
    }
    }

    {
    bplus_tree tree("test.db");
    assert(tree.meta.internal_node_num == 5);
    assert(tree.meta.leaf_node_num == 10);
    assert(tree.meta.height == 2);

    bpt::internal_node_t node1, node2, node3, node4, root;
    tree.map(&root, tree.meta.root_offset);
    off_t node1_off = tree.search_index("11");
    off_t node2_off = tree.search_index("22");
    off_t node3_off = tree.search_index("28");
    off_t node4_off = tree.search_index("6");
    tree.map(&node1, node1_off);
    tree.map(&node2, node2_off);
    tree.map(&node3, node3_off);
    tree.map(&node4, node4_off);
    assert(root.n == 3);
    assert(bpt::keycmp(root.children[0].key, "17") == 0);
    assert(bpt::keycmp(root.children[1].key, "25") == 0);
    assert(bpt::keycmp(root.children[2].key, "3") == 0);
    assert(root.children[0].child == node1_off);
    assert(root.children[1].child == node2_off);
    assert(root.children[2].child == node3_off);
    assert(root.children[3].child == node4_off);
    assert(node1.n == 2);
    assert(bpt::keycmp(node1.children[0].key, "11") == 0);
    assert(bpt::keycmp(node1.children[1].key, "14") == 0);
    assert(node2.n == 2);
    assert(bpt::keycmp(node2.children[0].key, "2") == 0);
    assert(bpt::keycmp(node2.children[1].key, "22") == 0);
    assert(node3.n == 1);
    assert(bpt::keycmp(node3.children[0].key, "28") == 0);
    assert(node4.n == 1);
    assert(bpt::keycmp(node4.children[0].key, "6") == 0);

    for (int i = 0; i < 30; i++) {
        char key[8] = { 0 };
        sprintf(key, "%d", i);
        bpt::value_t value;
        assert(tree.search(key, &value) == 0);
        assert(value == i);
    }
    PRINT("SplitAfterNewRoot");
    }

    {
    bplus_tree tree("test.db", true);
    for (int i = 0; i < 49; i++) {
        char key[8] = { 0 };
        sprintf(key, "%d", i);
        assert(tree.insert(key, i) == 0);
    }
    }

    {
    bplus_tree tree("test.db");
    assert(tree.meta.internal_node_num == 9);
    assert(tree.meta.leaf_node_num == 17);
    assert(tree.meta.height == 3);

    bpt::internal_node_t root;
    tree.map(&root, tree.meta.root_offset);
    assert(root.n == 1);
    assert(bpt::keycmp(root.children[0].key, "3") == 0);

    bpt::internal_node_t node1, node2;
    tree.map(&node1, root.children[0].child);
    tree.map(&node2, root.children[1].child);
    assert(node1.n == 2);
    assert(bpt::keycmp(node1.children[0].key, "17") == 0);
    assert(bpt::keycmp(node1.children[1].key, "25") == 0);
    assert(node2.n == 2);
    assert(bpt::keycmp(node2.children[0].key, "4") == 0);
    assert(bpt::keycmp(node2.children[1].key, "45") == 0);

    bpt::internal_node_t node3, node4, node5;
    tree.map(&node3, node2.children[0].child);
    assert(node3.n == 3);
    assert(bpt::keycmp(node3.children[0].key, "32") == 0);
    assert(bpt::keycmp(node3.children[1].key, "35") == 0);
    assert(bpt::keycmp(node3.children[2].key, "38") == 0);
    tree.map(&node4, node2.children[1].child);
    assert(node4.n == 1);
    assert(bpt::keycmp(node4.children[0].key, "42") == 0);
    tree.map(&node5, node2.children[2].child);
    assert(node5.n == 2);
    assert(bpt::keycmp(node5.children[0].key, "48") == 0);
    assert(bpt::keycmp(node5.children[1].key, "6") == 0);

    for (int i = 0; i < 49; i++) {
        char key[8] = { 0 };
        sprintf(key, "%d", i);
        bpt::value_t value;
        assert(tree.search(key, &value) == 0);
        assert(value == i);
    }
    PRINT("DanglingMiddleKey");
    }

    const int size = 128;
    int numbers[size];
    for (int i = 0; i < size; i++)
        numbers[i] = i;

    {
    bplus_tree tree("test.db", true);
    for (int i = 0; i < size; i++) {
        char key[8] = { 0 };
        sprintf(key, "%d", numbers[i]);
        assert(tree.insert(key, numbers[i]) == 0);
    }
    }

    {
    bplus_tree tree("test.db");
    for (int i = 0; i < size; i++) {
        char key[8] = { 0 };
        sprintf(key, "%d", numbers[i]);
        bpt::value_t value;
        assert(tree.search(key, &value) == 0);
        assert(value == numbers[i]);
    }
    PRINT("InsertManyKeys");
    }

    std::reverse(numbers, numbers + size);
    {
    bplus_tree tree("test.db", true);
    for (int i = 0; i < size; i++) {
        char key[8] = { 0 };
        sprintf(key, "%d", numbers[i]);
        assert(tree.insert(key, numbers[i]) == 0);
    }
    }

    {
    bplus_tree tree("test.db");
    for (int i = 0; i < size; i++) {
        char key[8] = { 0 };
        sprintf(key, "%d", numbers[i]);
        bpt::value_t value;
        assert(tree.search(key, &value) == 0);
        assert(value == numbers[i]);
    }
    PRINT("InsertManyKeysReverse");
    }

    for (int i = 0; i < 10; i++) {
        std::random_shuffle(numbers, numbers + size);
        {
        bplus_tree tree("test.db", true);
        for (int i = 0; i < size; i++) {
            char key[8] = { 0 };
            sprintf(key, "%d", numbers[i]);
            assert(tree.insert(key, numbers[i]) == 0);
        }
        }

        {
        bplus_tree tree("test.db");
        for (int i = 0; i < size; i++) {
            char key[8] = { 0 };
            sprintf(key, "%d", numbers[i]);
            bpt::value_t value;
            assert(tree.search(key, &value) == 0);
            assert(value == numbers[i]);
        }
        }
    }

    PRINT("InsertManyKeysRandom");

    for (int i = 0; i < 10; i++) {
        std::random_shuffle(numbers, numbers + size);
        {
        bplus_tree tree("test.db", true);
        for (int i = 0; i < size; i++) {
            char key[8] = { 0 };
            sprintf(key, "%d", numbers[i]);
            assert(tree.insert(key, numbers[i]) == 0);
        }
        }

        {
        bplus_tree tree("test.db");
        std::random_shuffle(numbers, numbers + size);
        for (int i = 0; i < size; i++) {
            char key[8] = { 0 };
            sprintf(key, "%d", numbers[i]);
            assert(tree.update(key, numbers[i] + 1) == 0);
        }
        }

        {
        bplus_tree tree("test.db");
        for (int i = 0; i < size; i++) {
            char key[8] = { 0 };
            sprintf(key, "%d", numbers[i]);
            bpt::value_t value;
            assert(tree.search(key, &value) == 0);
            assert(value == numbers[i] + 1);
        }

        for (int i = size; i < size * 2; i++) {
            char key[8] = { 0 };
            sprintf(key, "%d", i);
            bpt::value_t value;
            assert(tree.search(key, &value) != 0);
            assert(tree.update(key, i) != 0);
        }
        }
    }

    PRINT("UpdateManyKeysRandom");

    unlink("test.db");

    return 0;
}
