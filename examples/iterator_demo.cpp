#include "../include/BPlusTree.h"
#include <iostream>
#include <string>
#include <algorithm>

using namespace bptree;

int main() {
    std::cout << "B+ Tree Iterator Demo\n";
    std::cout << "=====================\n\n";

    // Create a B+ tree
    BPlusTree<int, std::string> tree(4);

    // Insert some data
    tree.insert(50, "fifty");
    tree.insert(20, "twenty");
    tree.insert(80, "eighty");
    tree.insert(10, "ten");
    tree.insert(30, "thirty");
    tree.insert(70, "seventy");
    tree.insert(90, "ninety");
    tree.insert(40, "forty");
    tree.insert(60, "sixty");

    // 1. Forward iteration using range-based for loop
    std::cout << "1. Forward iteration (range-based for):\n";
    for (const auto& pair : tree) {
        std::cout << "   " << pair.first << " => " << pair.second << "\n";
    }
    std::cout << "\n";

    // 2. Forward iteration using iterators
    std::cout << "2. Forward iteration (explicit iterators):\n";
    for (auto it = tree.begin(); it != tree.end(); ++it) {
        std::cout << "   " << it->first << " => " << it->second << "\n";
    }
    std::cout << "\n";

    // 3. Reverse iteration
    std::cout << "3. Reverse iteration:\n";
    for (auto it = tree.rbegin(); it != tree.rend(); ++it) {
        std::cout << "   " << it->first << " => " << it->second << "\n";
    }
    std::cout << "\n";

    // 4. Const iteration
    std::cout << "4. Const iteration:\n";
    const auto& const_tree = tree;
    for (auto it = const_tree.cbegin(); it != const_tree.cend(); ++it) {
        std::cout << "   " << it->first << " => " << it->second << "\n";
    }
    std::cout << "\n";

    // 5. Using STL algorithms with iterators
    std::cout << "5. Using STL algorithms:\n";

    // Count elements
    auto count = std::distance(tree.begin(), tree.end());
    std::cout << "   Total elements: " << count << "\n";

    // Find element with key 50
    auto it = std::find_if(tree.begin(), tree.end(),
                           [](const auto& pair) { return pair.first == 50; });
    if (it != tree.end()) {
        std::cout << "   Found key 50: " << it->second << "\n";
    }

    // Count elements with keys > 50
    auto count_gt_50 = std::count_if(tree.begin(), tree.end(),
                                      [](const auto& pair) { return pair.first > 50; });
    std::cout << "   Elements with key > 50: " << count_gt_50 << "\n";

    // Check if all elements have non-empty values
    bool all_non_empty = std::all_of(tree.begin(), tree.end(),
                                      [](const auto& pair) { return !pair.second.empty(); });
    std::cout << "   All values non-empty: " << (all_non_empty ? "yes" : "no") << "\n";
    std::cout << "\n";

    // 6. Bidirectional iteration
    std::cout << "6. Bidirectional iteration (forward then back):\n";
    auto fwd_it = tree.begin();
    std::cout << "   Start: " << fwd_it->first << "\n";
    ++fwd_it;
    std::cout << "   Next:  " << fwd_it->first << "\n";
    ++fwd_it;
    std::cout << "   Next:  " << fwd_it->first << "\n";
    --fwd_it;
    std::cout << "   Prev:  " << fwd_it->first << "\n";
    --fwd_it;
    std::cout << "   Prev:  " << fwd_it->first << "\n";
    std::cout << "\n";

    // 7. Manual loop with iterator comparison
    std::cout << "7. Manual iteration with comparison:\n";
    auto manual_it = tree.begin();
    auto end_it = tree.end();
    int limit = 3;
    int printed = 0;
    while (manual_it != end_it && printed < limit) {
        std::cout << "   " << manual_it->first << " => " << manual_it->second << "\n";
        ++manual_it;
        ++printed;
    }
    std::cout << "   ... (showing first " << limit << " elements)\n";
    std::cout << "\n";

    // 8. Reverse iteration with bidirectional movement
    std::cout << "8. Reverse iterator bidirectional movement:\n";
    auto rev_it = tree.rbegin();
    std::cout << "   Start (last element): " << rev_it->first << "\n";
    ++rev_it;
    std::cout << "   Next (backward):      " << rev_it->first << "\n";
    ++rev_it;
    std::cout << "   Next (backward):      " << rev_it->first << "\n";
    --rev_it;
    std::cout << "   Prev (forward):       " << rev_it->first << "\n";
    std::cout << "\n";

    // 9. Collecting elements into a container
    std::cout << "9. Collecting keys into a vector:\n";
    std::vector<int> keys;
    for (const auto& pair : tree) {
        keys.push_back(pair.first);
    }
    std::cout << "   Keys: ";
    for (int key : keys) {
        std::cout << key << " ";
    }
    std::cout << "\n\n";

    // 10. Using std::for_each
    std::cout << "10. Using std::for_each:\n";
    std::for_each(tree.begin(), tree.end(),
                  [](const auto& pair) {
                      std::cout << "    Key: " << pair.first
                                << ", Value length: " << pair.second.length() << "\n";
                  });
    std::cout << "\n";

    std::cout << "Demo completed successfully!\n";
    return 0;
}
