#include "../include/BPlusTree.h"
#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>

using namespace bptree;

void testBulkLoadEmpty() {
    BPlusTree<int, std::string> tree(4);
    std::vector<std::pair<int, std::string>> data;

    tree.bulkLoad(data);

    assert(tree.isEmpty());
    assert(tree.validate());

    std::cout << "✓ Empty bulk load test passed" << std::endl;
}

void testBulkLoadSingleElement() {
    BPlusTree<int, std::string> tree(4);
    std::vector<std::pair<int, std::string>> data = {{10, "value10"}};

    tree.bulkLoad(data);

    std::string value;
    assert(tree.search(10, value));
    assert(value == "value10");
    assert(tree.validate());

    std::cout << "✓ Single element bulk load test passed" << std::endl;
}

void testBulkLoadSmall() {
    BPlusTree<int, std::string> tree(4);
    std::vector<std::pair<int, std::string>> data = {
        {1, "one"},
        {2, "two"},
        {3, "three"},
        {4, "four"},
        {5, "five"}
    };

    tree.bulkLoad(data);

    std::string value;
    for (const auto& pair : data) {
        assert(tree.search(pair.first, value));
        assert(value == pair.second);
    }
    assert(tree.validate());

    std::cout << "✓ Small bulk load test passed" << std::endl;
}

void testBulkLoadMedium() {
    BPlusTree<int, int> tree(4);
    std::vector<std::pair<int, int>> data;

    // Create sorted data
    for (int i = 0; i < 100; i++) {
        data.emplace_back(i, i * 2);
    }

    tree.bulkLoad(data);

    // Verify all data
    int value;
    for (int i = 0; i < 100; i++) {
        assert(tree.search(i, value));
        assert(value == i * 2);
    }
    assert(tree.validate());

    std::cout << "✓ Medium bulk load test passed" << std::endl;
}

void testBulkLoadLarge() {
    BPlusTree<int, int> tree(5);
    std::vector<std::pair<int, int>> data;

    // Create sorted data
    for (int i = 0; i < 10000; i++) {
        data.emplace_back(i, i * 3);
    }

    tree.bulkLoad(data);

    // Verify all data
    int value;
    for (int i = 0; i < 10000; i++) {
        assert(tree.search(i, value));
        assert(value == i * 3);
    }
    assert(tree.validate());

    std::cout << "✓ Large bulk load test passed" << std::endl;
}

void testBulkLoadWithDuplicates() {
    BPlusTree<int, std::string> tree(4);
    std::vector<std::pair<int, std::string>> data = {
        {1, "first1"},
        {1, "second1"},  // duplicate - should overwrite
        {2, "first2"},
        {3, "first3"},
        {3, "second3"},  // duplicate - should overwrite
        {3, "third3"},   // duplicate - should overwrite
        {4, "first4"}
    };

    tree.bulkLoad(data);

    std::string value;
    assert(tree.search(1, value));
    assert(value == "second1");  // Last value for key 1

    assert(tree.search(2, value));
    assert(value == "first2");

    assert(tree.search(3, value));
    assert(value == "third3");  // Last value for key 3

    assert(tree.search(4, value));
    assert(value == "first4");

    assert(tree.validate());

    std::cout << "✓ Bulk load with duplicates test passed" << std::endl;
}

void testBulkLoadOverwriteExisting() {
    BPlusTree<int, std::string> tree(4);

    // Insert some data first
    tree.insert(100, "old100");
    tree.insert(200, "old200");

    // Bulk load should clear existing data
    std::vector<std::pair<int, std::string>> data = {
        {1, "new1"},
        {2, "new2"},
        {3, "new3"}
    };

    tree.bulkLoad(data);

    // Old data should be gone
    std::string value;
    assert(!tree.search(100, value));
    assert(!tree.search(200, value));

    // New data should exist
    assert(tree.search(1, value));
    assert(value == "new1");
    assert(tree.search(2, value));
    assert(value == "new2");
    assert(tree.search(3, value));
    assert(value == "new3");

    assert(tree.validate());

    std::cout << "✓ Bulk load overwrite existing test passed" << std::endl;
}

void testBulkLoadIteratorRange() {
    BPlusTree<int, int> tree(4);
    std::vector<std::pair<int, int>> data;

    for (int i = 0; i < 50; i++) {
        data.emplace_back(i, i * 10);
    }

    // Load only a partial range
    tree.bulkLoad(data.begin() + 10, data.begin() + 30);

    // Verify only the loaded range exists
    int value;
    for (int i = 0; i < 10; i++) {
        assert(!tree.search(i, value));
    }
    for (int i = 10; i < 30; i++) {
        assert(tree.search(i, value));
        assert(value == i * 10);
    }
    for (int i = 30; i < 50; i++) {
        assert(!tree.search(i, value));
    }

    assert(tree.validate());

    std::cout << "✓ Bulk load iterator range test passed" << std::endl;
}

void testBulkLoadRangeQuery() {
    BPlusTree<int, int> tree(4);
    std::vector<std::pair<int, int>> data;

    for (int i = 0; i < 100; i++) {
        data.emplace_back(i, i);
    }

    tree.bulkLoad(data);

    // Test range query on bulk loaded tree
    auto result = tree.rangeQuery(25, 35);
    assert(result.size() == 11);
    for (int i = 0; i < 11; i++) {
        assert(result[i].first == 25 + i);
        assert(result[i].second == 25 + i);
    }

    assert(tree.validate());

    std::cout << "✓ Bulk load range query test passed" << std::endl;
}

void testBulkLoadIterator() {
    BPlusTree<int, std::string> tree(4);
    std::vector<std::pair<int, std::string>> data = {
        {1, "a"}, {2, "b"}, {3, "c"}, {4, "d"}, {5, "e"},
        {6, "f"}, {7, "g"}, {8, "h"}, {9, "i"}, {10, "j"}
    };

    tree.bulkLoad(data);

    // Test forward iteration
    size_t count = 0;
    int expectedKey = 1;
    for (const auto& pair : tree) {
        assert(pair.first == expectedKey);
        expectedKey++;
        count++;
    }
    assert(count == 10);

    // Test reverse iteration
    count = 0;
    expectedKey = 10;
    for (auto it = tree.rbegin(); it != tree.rend(); ++it) {
        assert(it->first == expectedKey);
        expectedKey--;
        count++;
    }
    assert(count == 10);

    assert(tree.validate());

    std::cout << "✓ Bulk load iterator test passed" << std::endl;
}

void testBulkLoadMoveSemantics() {
    BPlusTree<int, std::string> tree(4);
    std::vector<std::pair<int, std::string>> data = {
        {1, "one"},
        {2, "two"},
        {3, "three"}
    };

    // Use the move overload
    tree.bulkLoad(std::move(data));

    std::string value;
    assert(tree.search(1, value));
    assert(value == "one");
    assert(tree.search(2, value));
    assert(value == "two");
    assert(tree.search(3, value));
    assert(value == "three");

    assert(tree.validate());

    std::cout << "✓ Bulk load move semantics test passed" << std::endl;
}

void testBulkLoadDifferentOrders() {
    // Test with different tree orders
    for (size_t order = 3; order <= 10; order++) {
        BPlusTree<int, int> tree(order);
        std::vector<std::pair<int, int>> data;

        for (int i = 0; i < 500; i++) {
            data.emplace_back(i, i);
        }

        tree.bulkLoad(data);

        int value;
        for (int i = 0; i < 500; i++) {
            assert(tree.search(i, value));
            assert(value == i);
        }
        assert(tree.validate());
    }

    std::cout << "✓ Bulk load different orders test passed" << std::endl;
}

void testBulkLoadModifyAfter() {
    BPlusTree<int, int> tree(4);
    std::vector<std::pair<int, int>> data;

    for (int i = 0; i < 50; i++) {
        data.emplace_back(i * 2, i * 2);  // Even numbers: 0, 2, 4, ..., 98
    }

    tree.bulkLoad(data);

    // Insert odd numbers after bulk loading
    for (int i = 0; i < 50; i++) {
        tree.insert(i * 2 + 1, i * 2 + 1);  // Odd numbers: 1, 3, 5, ..., 99
    }

    // Verify all 100 elements
    int value;
    for (int i = 0; i < 100; i++) {
        assert(tree.search(i, value));
        assert(value == i);
    }

    // Delete some elements
    for (int i = 0; i < 25; i++) {
        assert(tree.remove(i * 4));  // Remove 0, 4, 8, 12, ..., 96
    }

    // Verify remaining elements
    for (int i = 0; i < 100; i++) {
        if (i % 4 == 0) {
            assert(!tree.search(i, value));
        } else {
            assert(tree.search(i, value));
            assert(value == i);
        }
    }

    assert(tree.validate());

    std::cout << "✓ Bulk load modify after test passed" << std::endl;
}

void testBulkLoadPerformanceComparison() {
    const int NUM_ELEMENTS = 100000;

    // Prepare sorted data
    std::vector<std::pair<int, int>> data;
    for (int i = 0; i < NUM_ELEMENTS; i++) {
        data.emplace_back(i, i);
    }

    // Measure bulk load time
    BPlusTree<int, int> tree1(100);
    auto start1 = std::chrono::high_resolution_clock::now();
    tree1.bulkLoad(data);
    auto end1 = std::chrono::high_resolution_clock::now();
    auto bulkTime = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();

    // Measure sequential insert time
    BPlusTree<int, int> tree2(100);
    auto start2 = std::chrono::high_resolution_clock::now();
    for (const auto& pair : data) {
        tree2.insert(pair.first, pair.second);
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    auto insertTime = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count();

    // Verify both trees are valid and contain same data
    assert(tree1.validate());
    assert(tree2.validate());

    int value;
    for (int i = 0; i < NUM_ELEMENTS; i++) {
        assert(tree1.search(i, value) && value == i);
        assert(tree2.search(i, value) && value == i);
    }

    std::cout << "✓ Bulk load performance comparison test passed" << std::endl;
    std::cout << "  Bulk load: " << bulkTime << "ms, Sequential insert: " << insertTime << "ms" << std::endl;
    std::cout << "  Speedup: " << (insertTime > 0 ? static_cast<double>(insertTime) / bulkTime : 0) << "x" << std::endl;
}

void testBulkLoadStringKeys() {
    BPlusTree<std::string, int> tree(4);
    std::vector<std::pair<std::string, int>> data = {
        {"apple", 1},
        {"banana", 2},
        {"cherry", 3},
        {"date", 4},
        {"elderberry", 5}
    };

    tree.bulkLoad(data);

    int value;
    assert(tree.search("apple", value) && value == 1);
    assert(tree.search("banana", value) && value == 2);
    assert(tree.search("cherry", value) && value == 3);
    assert(tree.search("date", value) && value == 4);
    assert(tree.search("elderberry", value) && value == 5);

    assert(tree.validate());

    std::cout << "✓ Bulk load string keys test passed" << std::endl;
}

int main() {
    std::cout << "Running bulk load tests..." << std::endl;

    testBulkLoadEmpty();
    testBulkLoadSingleElement();
    testBulkLoadSmall();
    testBulkLoadMedium();
    testBulkLoadLarge();
    testBulkLoadWithDuplicates();
    testBulkLoadOverwriteExisting();
    testBulkLoadIteratorRange();
    testBulkLoadRangeQuery();
    testBulkLoadIterator();
    testBulkLoadMoveSemantics();
    testBulkLoadDifferentOrders();
    testBulkLoadModifyAfter();
    testBulkLoadPerformanceComparison();
    testBulkLoadStringKeys();

    std::cout << "\n✓ All bulk load tests passed!" << std::endl;
    return 0;
}
