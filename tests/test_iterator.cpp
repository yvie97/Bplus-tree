#include "../include/BPlusTree.h"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>

using namespace bptree;

void testEmptyTreeIterator() {
    BPlusTree<int, std::string> tree(4);

    // Test that begin() == end() for empty tree
    assert(tree.begin() == tree.end());
    assert(tree.cbegin() == tree.cend());

    // Test that no iteration occurs for empty tree
    int count = 0;
    for (auto it = tree.begin(); it != tree.end(); ++it) {
        count++;
    }
    assert(count == 0);

    std::cout << "✓ Empty tree iterator test passed" << std::endl;
}

void testSingleElementIteration() {
    BPlusTree<int, std::string> tree(4);
    tree.insert(10, "value10");

    // Test forward iteration
    auto it = tree.begin();
    assert(it != tree.end());
    assert(it->first == 10);
    assert(it->second == "value10");

    ++it;
    assert(it == tree.end());

    std::cout << "✓ Single element iteration test passed" << std::endl;
}

void testMultipleElementIteration() {
    BPlusTree<int, std::string> tree(4);

    // Insert elements in random order
    std::vector<int> keys = {50, 20, 80, 10, 30, 70, 90, 40, 60};
    for (int key : keys) {
        tree.insert(key, "value" + std::to_string(key));
    }

    // Verify iteration is in sorted order
    std::vector<int> expected = {10, 20, 30, 40, 50, 60, 70, 80, 90};
    std::vector<int> actual;

    for (auto it = tree.begin(); it != tree.end(); ++it) {
        actual.push_back(it->first);
        assert(it->second == "value" + std::to_string(it->first));
    }

    assert(actual == expected);
    std::cout << "✓ Multiple element iteration test passed" << std::endl;
}

void testRangeBasedForLoop() {
    BPlusTree<int, std::string> tree(4);

    // Insert elements
    for (int i = 1; i <= 10; i++) {
        tree.insert(i * 10, "value" + std::to_string(i * 10));
    }

    // Test range-based for loop
    int count = 0;
    int expected_key = 10;
    for (const auto& pair : tree) {
        assert(pair.first == expected_key);
        assert(pair.second == "value" + std::to_string(expected_key));
        expected_key += 10;
        count++;
    }
    assert(count == 10);

    std::cout << "✓ Range-based for loop test passed" << std::endl;
}

void testConstIterator() {
    BPlusTree<int, std::string> tree(4);

    for (int i = 1; i <= 5; i++) {
        tree.insert(i, "value" + std::to_string(i));
    }

    // Test const iteration
    const auto& const_tree = tree;
    int count = 0;
    int expected_key = 1;

    for (auto it = const_tree.cbegin(); it != const_tree.cend(); ++it) {
        assert(it->first == expected_key);
        assert(it->second == "value" + std::to_string(expected_key));
        expected_key++;
        count++;
    }
    assert(count == 5);

    std::cout << "✓ Const iterator test passed" << std::endl;
}

void testIteratorIncrement() {
    BPlusTree<int, std::string> tree(4);

    for (int i = 1; i <= 5; i++) {
        tree.insert(i, "value" + std::to_string(i));
    }

    auto it = tree.begin();

    // Test pre-increment
    assert(it->first == 1);
    ++it;
    assert(it->first == 2);
    ++it;
    assert(it->first == 3);

    // Test post-increment
    auto old_it = it++;
    assert(old_it->first == 3);
    assert(it->first == 4);

    std::cout << "✓ Iterator increment test passed" << std::endl;
}

void testIteratorDecrement() {
    BPlusTree<int, std::string> tree(4);

    for (int i = 1; i <= 5; i++) {
        tree.insert(i, "value" + std::to_string(i));
    }

    // Start from an iterator in the middle
    auto it = tree.begin();
    ++it; ++it; ++it; ++it; // Now at element 5

    assert(it->first == 5);

    // Test pre-decrement
    --it;
    assert(it->first == 4);
    --it;
    assert(it->first == 3);

    // Test post-decrement
    auto old_it = it--;
    assert(old_it->first == 3);
    assert(it->first == 2);

    std::cout << "✓ Iterator decrement test passed" << std::endl;
}

void testReverseIterator() {
    BPlusTree<int, std::string> tree(4);

    // Insert elements
    for (int i = 1; i <= 5; i++) {
        tree.insert(i * 10, "value" + std::to_string(i * 10));
    }

    // Test reverse iteration
    std::vector<int> expected = {50, 40, 30, 20, 10};
    std::vector<int> actual;

    for (auto it = tree.rbegin(); it != tree.rend(); ++it) {
        actual.push_back(it->first);
    }

    assert(actual == expected);
    std::cout << "✓ Reverse iterator test passed" << std::endl;
}

void testConstReverseIterator() {
    BPlusTree<int, std::string> tree(4);

    for (int i = 1; i <= 5; i++) {
        tree.insert(i * 10, "value" + std::to_string(i * 10));
    }

    const auto& const_tree = tree;
    std::vector<int> expected = {50, 40, 30, 20, 10};
    std::vector<int> actual;

    for (auto it = const_tree.crbegin(); it != const_tree.crend(); ++it) {
        actual.push_back(it->first);
    }

    assert(actual == expected);
    std::cout << "✓ Const reverse iterator test passed" << std::endl;
}

void testIteratorEquality() {
    BPlusTree<int, std::string> tree(4);

    for (int i = 1; i <= 5; i++) {
        tree.insert(i, "value" + std::to_string(i));
    }

    auto it1 = tree.begin();
    auto it2 = tree.begin();

    // Test equality
    assert(it1 == it2);
    assert(!(it1 != it2));

    // Test inequality after increment
    ++it2;
    assert(it1 != it2);
    assert(!(it1 == it2));

    // Test equality after matching increments
    ++it1;
    assert(it1 == it2);

    std::cout << "✓ Iterator equality test passed" << std::endl;
}

void testIteratorWithSTLAlgorithms() {
    BPlusTree<int, std::string> tree(4);

    for (int i = 1; i <= 10; i++) {
        tree.insert(i, "value" + std::to_string(i));
    }

    // Test std::distance
    auto dist = std::distance(tree.begin(), tree.end());
    assert(dist == 10);

    // Test std::find_if
    auto it = std::find_if(tree.begin(), tree.end(),
                           [](const auto& pair) { return pair.first == 5; });
    assert(it != tree.end());
    assert(it->first == 5);
    assert(it->second == "value5");

    // Test std::count_if
    auto count = std::count_if(tree.begin(), tree.end(),
                               [](const auto& pair) { return pair.first > 5; });
    assert(count == 5);

    // Test std::all_of
    bool all_positive = std::all_of(tree.begin(), tree.end(),
                                    [](const auto& pair) { return pair.first > 0; });
    assert(all_positive);

    std::cout << "✓ Iterator with STL algorithms test passed" << std::endl;
}

void testIteratorAcrossLeafBoundaries() {
    // Use smaller order to force multiple leaf nodes
    BPlusTree<int, std::string> tree(3); // order 3 = max 2 keys per node

    // Insert enough elements to create multiple leaf nodes
    for (int i = 1; i <= 20; i++) {
        tree.insert(i, "value" + std::to_string(i));
    }

    // Verify iteration across leaf boundaries
    int count = 0;
    int expected_key = 1;
    for (auto it = tree.begin(); it != tree.end(); ++it) {
        assert(it->first == expected_key);
        expected_key++;
        count++;
    }
    assert(count == 20);

    std::cout << "✓ Iterator across leaf boundaries test passed" << std::endl;
}

void testIteratorAfterModification() {
    BPlusTree<int, std::string> tree(4);

    // Insert initial elements
    for (int i = 1; i <= 5; i++) {
        tree.insert(i * 10, "value" + std::to_string(i * 10));
    }

    // Insert more elements
    tree.insert(25, "value25");
    tree.insert(35, "value35");

    // Verify iteration includes new elements in correct order
    std::vector<int> expected = {10, 20, 25, 30, 35, 40, 50};
    std::vector<int> actual;

    for (const auto& pair : tree) {
        actual.push_back(pair.first);
    }

    assert(actual == expected);

    // Remove some elements
    tree.remove(25);
    tree.remove(40);

    // Verify iteration after deletion
    expected = {10, 20, 30, 35, 50};
    actual.clear();

    for (const auto& pair : tree) {
        actual.push_back(pair.first);
    }

    assert(actual == expected);
    std::cout << "✓ Iterator after modification test passed" << std::endl;
}

void testIteratorDereference() {
    BPlusTree<int, std::string> tree(4);
    tree.insert(10, "value10");
    tree.insert(20, "value20");

    auto it = tree.begin();

    // Test arrow operator
    assert(it->first == 10);
    assert(it->second == "value10");

    // Test dereference operator
    auto pair = *it;
    assert(pair.first == 10);
    assert(pair.second == "value10");

    std::cout << "✓ Iterator dereference test passed" << std::endl;
}

void testLargeTreeIteration() {
    BPlusTree<int, std::string> tree(4);

    // Insert many elements
    const int N = 1000;
    for (int i = 0; i < N; i++) {
        tree.insert(i, "value" + std::to_string(i));
    }

    // Verify all elements are iterated in order
    int count = 0;
    int expected = 0;
    for (const auto& pair : tree) {
        assert(pair.first == expected);
        expected++;
        count++;
    }
    assert(count == N);

    std::cout << "✓ Large tree iteration test passed" << std::endl;
}

int main() {
    std::cout << "Running B+ Tree Iterator Tests..." << std::endl;
    std::cout << "=================================" << std::endl;

    testEmptyTreeIterator();
    testSingleElementIteration();
    testMultipleElementIteration();
    testRangeBasedForLoop();
    testConstIterator();
    testIteratorIncrement();
    testIteratorDecrement();
    testReverseIterator();
    testConstReverseIterator();
    testIteratorEquality();
    testIteratorWithSTLAlgorithms();
    testIteratorAcrossLeafBoundaries();
    testIteratorAfterModification();
    testIteratorDereference();
    testLargeTreeIteration();

    std::cout << "=================================" << std::endl;
    std::cout << "All iterator tests passed!" << std::endl;

    return 0;
}
