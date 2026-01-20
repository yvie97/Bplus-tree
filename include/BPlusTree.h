#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#include "Node.h"
#include "Config.h"
#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <queue>
#include <cassert>
#include <iterator>

namespace bptree {

// Forward declaration
template<typename KeyType, typename ValueType>
class BPlusTree;

/**
 * @brief STL-compatible bidirectional iterator for B+ tree
 *
 * This iterator traverses the B+ tree in sorted key order by following
 * the linked list of leaf nodes. It provides standard iterator operations
 * including increment, decrement, dereference, and comparison.
 *
 * @tparam KeyType The type of keys in the tree
 * @tparam ValueType The type of values in the tree
 * @tparam IsConst Whether this is a const iterator
 */
template<typename KeyType, typename ValueType, bool IsConst = false>
class BPlusTreeIterator {
public:
    // STL iterator traits
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = std::pair<KeyType, ValueType>;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;

    // Internal types
    using leaf_node_type = typename std::conditional<IsConst,
                                                      const LeafNode<KeyType, ValueType>,
                                                      LeafNode<KeyType, ValueType>>::type;

private:
    leaf_node_type* current_leaf;  ///< Pointer to current leaf node
    size_t index;                  ///< Current index within the leaf node
    mutable value_type cached_pair; ///< Mutable cache for dereference operations

    friend class BPlusTree<KeyType, ValueType>;

    /**
     * @brief Private constructor for creating iterators
     * @param leaf Pointer to a leaf node
     * @param idx Index within the leaf node
     */
    BPlusTreeIterator(leaf_node_type* leaf, size_t idx)
        : current_leaf(leaf), index(idx), cached_pair() {}

public:
    /**
     * @brief Default constructor creates an end iterator
     */
    BPlusTreeIterator() : current_leaf(nullptr), index(0), cached_pair() {}

    /**
     * @brief Copy constructor
     */
    BPlusTreeIterator(const BPlusTreeIterator& other) = default;

    /**
     * @brief Copy assignment operator
     */
    BPlusTreeIterator& operator=(const BPlusTreeIterator& other) = default;

    /**
     * @brief Conversion from non-const to const iterator
     */
    template<bool WasConst = IsConst>
    BPlusTreeIterator(const BPlusTreeIterator<KeyType, ValueType, false>& other,
                      typename std::enable_if<WasConst>::type* = nullptr)
        : current_leaf(other.current_leaf), index(other.index), cached_pair() {}

    /**
     * @brief Pre-increment operator (++it)
     * @return Reference to this iterator after incrementing
     */
    BPlusTreeIterator& operator++() {
        if (!current_leaf) return *this;

        index++;
        if (index >= current_leaf->numKeys) {
            if (current_leaf->next) {
                // Move to next leaf node
                current_leaf = current_leaf->next;
                index = 0;
            }
            // else: stay at end position (index == numKeys on last leaf)
        }
        return *this;
    }

    /**
     * @brief Post-increment operator (it++)
     * @return Copy of iterator before incrementing
     */
    BPlusTreeIterator operator++(int) {
        BPlusTreeIterator temp = *this;
        ++(*this);
        return temp;
    }

    /**
     * @brief Pre-decrement operator (--it)
     * @return Reference to this iterator after decrementing
     */
    BPlusTreeIterator& operator--() {
        if (index > 0) {
            index--;
        } else if (current_leaf && current_leaf->prev) {
            // Move to previous leaf node
            current_leaf = current_leaf->prev;
            index = current_leaf->numKeys - 1;
        }
        return *this;
    }

    /**
     * @brief Post-decrement operator (it--)
     * @return Copy of iterator before decrementing
     */
    BPlusTreeIterator operator--(int) {
        BPlusTreeIterator temp = *this;
        --(*this);
        return temp;
    }

    /**
     * @brief Dereference operator
     * @return Reference to key-value pair at current position
     */
    reference operator*() const {
        cached_pair.first = current_leaf->keys[index];
        cached_pair.second = current_leaf->values[index];
        return cached_pair;
    }

    /**
     * @brief Arrow operator
     * @return Pointer to key-value pair at current position
     */
    pointer operator->() const {
        cached_pair.first = current_leaf->keys[index];
        cached_pair.second = current_leaf->values[index];
        return &cached_pair;
    }

    /**
     * @brief Equality comparison
     * @param other Iterator to compare with
     * @return true if iterators point to the same position
     */
    bool operator==(const BPlusTreeIterator& other) const {
        return current_leaf == other.current_leaf && index == other.index;
    }

    /**
     * @brief Inequality comparison
     * @param other Iterator to compare with
     * @return true if iterators point to different positions
     */
    bool operator!=(const BPlusTreeIterator& other) const {
        return !(*this == other);
    }

    template<typename K, typename V, bool C>
    friend class BPlusTreeIterator;
};

// Forward declaration of BPlusTree for friend declaration
template<typename KeyType, typename ValueType>
class BPlusTree;

/**
 * @brief Custom reverse iterator for B+ tree
 *
 * This reverse iterator is implemented separately from std::reverse_iterator
 * to avoid issues with dangling references from cached pairs.
 */
template<typename KeyType, typename ValueType, bool IsConst = false>
class BPlusTreeReverseIterator {
public:
    // STL iterator traits
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = std::pair<KeyType, ValueType>;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = const value_type&;

    // Internal types
    using leaf_node_type = typename std::conditional<IsConst,
                                                      const LeafNode<KeyType, ValueType>,
                                                      LeafNode<KeyType, ValueType>>::type;

private:
    leaf_node_type* current_leaf;
    size_t index;  // Points to actual element (not one-past like forward iterator's end)
    mutable value_type cached_pair;

    friend class BPlusTree<KeyType, ValueType>;

    BPlusTreeReverseIterator(leaf_node_type* leaf, size_t idx)
        : current_leaf(leaf), index(idx), cached_pair() {}

public:
    BPlusTreeReverseIterator() : current_leaf(nullptr), index(0), cached_pair() {}

    BPlusTreeReverseIterator(const BPlusTreeReverseIterator& other) = default;
    BPlusTreeReverseIterator& operator=(const BPlusTreeReverseIterator& other) = default;

    template<bool WasConst = IsConst>
    BPlusTreeReverseIterator(const BPlusTreeReverseIterator<KeyType, ValueType, false>& other,
                             typename std::enable_if<WasConst>::type* = nullptr)
        : current_leaf(other.current_leaf), index(other.index), cached_pair() {}

    // Increment moves backwards through the tree
    BPlusTreeReverseIterator& operator++() {
        if (!current_leaf) return *this;

        if (index > 0) {
            index--;
        } else if (current_leaf->prev) {
            current_leaf = current_leaf->prev;
            if (current_leaf->numKeys > 0) {
                index = current_leaf->numKeys - 1;
            }
        } else {
            // Reached the beginning, set to "end" state
            current_leaf = nullptr;
            index = 0;
        }
        return *this;
    }

    BPlusTreeReverseIterator operator++(int) {
        BPlusTreeReverseIterator temp = *this;
        ++(*this);
        return temp;
    }

    // Decrement moves forward through the tree
    BPlusTreeReverseIterator& operator--() {
        if (!current_leaf) return *this;

        index++;
        if (index >= current_leaf->numKeys) {
            if (current_leaf->next) {
                current_leaf = current_leaf->next;
                index = 0;
            } else {
                // Can't go past the last element
                index = current_leaf->numKeys - 1;
            }
        }
        return *this;
    }

    BPlusTreeReverseIterator operator--(int) {
        BPlusTreeReverseIterator temp = *this;
        --(*this);
        return temp;
    }

    reference operator*() const {
        cached_pair.first = current_leaf->keys[index];
        cached_pair.second = current_leaf->values[index];
        return cached_pair;
    }

    pointer operator->() const {
        cached_pair.first = current_leaf->keys[index];
        cached_pair.second = current_leaf->values[index];
        return &cached_pair;
    }

    bool operator==(const BPlusTreeReverseIterator& other) const {
        return current_leaf == other.current_leaf && index == other.index;
    }

    bool operator!=(const BPlusTreeReverseIterator& other) const {
        return !(*this == other);
    }

    template<typename K, typename V, bool C>
    friend class BPlusTreeReverseIterator;
};

/**
 * @brief B+ Tree implementation with exception safety guarantees
 *
 * A B+ Tree is a self-balancing tree data structure that maintains sorted data
 * and allows searches, insertions, deletions, and sequential access in logarithmic time.
 * All data is stored in leaf nodes, which are linked for efficient range queries.
 *
 * @tparam KeyType The type of keys stored in the tree. Must support comparison operators (<, >=, ==).
 * @tparam ValueType The type of values associated with keys.
 *
 * Exception Safety Guarantees:
 * - Constructor: Strong guarantee - either succeeds or no tree is created
 * - Destructor: No-throw guarantee (noexcept)
 * - Move operations: No-throw guarantee (noexcept)
 * - search(): No-throw guarantee if KeyType and ValueType operations don't throw
 * - insert(): Basic guarantee - tree remains valid but may be partially modified
 *   if KeyType or ValueType copy/move operations throw
 * - remove(): Basic guarantee - tree remains valid
 * - rangeQuery(): Strong guarantee - either returns results or leaves tree unchanged
 *
 * Requirements for KeyType and ValueType:
 * - Must be copyable or movable
 * - Copy/move operations may throw, but should leave objects in valid state
 * - For best exception safety, prefer move semantics
 */
template<typename KeyType, typename ValueType>
class BPlusTree {
public:
    // Iterator type definitions
    using iterator = BPlusTreeIterator<KeyType, ValueType, false>;
    using const_iterator = BPlusTreeIterator<KeyType, ValueType, true>;
    using reverse_iterator = BPlusTreeReverseIterator<KeyType, ValueType, false>;
    using const_reverse_iterator = BPlusTreeReverseIterator<KeyType, ValueType, true>;

private:
    Node<KeyType, ValueType>* root;
    size_t order;      // m
    size_t maxKeys;    // m - 1
    size_t minKeys;    // ⌈m/2⌉ - 1

    // Helper functions
    LeafNode<KeyType, ValueType>* findLeaf(const KeyType& key);
    const LeafNode<KeyType, ValueType>* findLeaf(const KeyType& key) const;
    void splitLeaf(LeafNode<KeyType, ValueType>* leaf);
    void splitInternal(InternalNode<KeyType, ValueType>* node);
    void insertIntoParent(Node<KeyType, ValueType>* left, const KeyType& key,
                          Node<KeyType, ValueType>* right);

    void deleteEntry(Node<KeyType, ValueType>* node);
    void mergeNodes(Node<KeyType, ValueType>* node, Node<KeyType, ValueType>* sibling,
                    int parentIndex, bool isLeftSibling);
    void redistributeNodes(Node<KeyType, ValueType>* node, Node<KeyType, ValueType>* sibling,
                           int parentIndex, bool isLeftSibling);
    int getNodeIndex(Node<KeyType, ValueType>* node);

    void destroyTree(Node<KeyType, ValueType>* node);
    void printNode(const Node<KeyType, ValueType>* node, int level) const;
    bool validateNode(const Node<KeyType, ValueType>* node, int level, int& leafLevel) const;

    // Helper methods for iterators
    LeafNode<KeyType, ValueType>* getFirstLeaf();
    const LeafNode<KeyType, ValueType>* getFirstLeaf() const;
    LeafNode<KeyType, ValueType>* getLastLeaf();
    const LeafNode<KeyType, ValueType>* getLastLeaf() const;

public:
    /**
     * @brief Constructs a B+ Tree with the specified order
     *
     * @param order The order of the B+ tree (maximum number of children per node).
     *              Must be at least MIN_ORDER (3). If a smaller value is provided,
     *              MIN_ORDER will be used instead. Default is DEFAULT_ORDER (4).
     *
     * Time complexity: O(1)
     * Space complexity: O(1)
     * Exception safety: Strong guarantee
     */
    explicit BPlusTree(size_t order = DEFAULT_ORDER);

    /**
     * @brief Destroys the B+ Tree and deallocates all nodes
     *
     * Time complexity: O(n) where n is the number of nodes
     * Exception safety: No-throw guarantee (noexcept)
     */
    ~BPlusTree();

    /**
     * @brief Deleted copy constructor
     *
     * Copying a tree is expensive and usually unintended. Use move semantics instead.
     */
    BPlusTree(const BPlusTree&) = delete;

    /**
     * @brief Deleted copy assignment operator
     *
     * Copying a tree is expensive and usually unintended. Use move semantics instead.
     */
    BPlusTree& operator=(const BPlusTree&) = delete;

    /**
     * @brief Move constructor for efficient transfer of ownership
     *
     * @param other The tree to move from. After the move, other will be in a valid
     *              but empty state.
     *
     * Time complexity: O(1)
     * Exception safety: No-throw guarantee (noexcept)
     */
    BPlusTree(BPlusTree&& other) noexcept;

    /**
     * @brief Move assignment operator for efficient transfer of ownership
     *
     * @param other The tree to move from. After the move, other will be in a valid
     *              but empty state.
     * @return Reference to this tree
     *
     * Time complexity: O(n) where n is the number of nodes in the current tree (for cleanup)
     * Exception safety: No-throw guarantee (noexcept)
     */
    BPlusTree& operator=(BPlusTree&& other) noexcept;

    /**
     * @brief Searches for a key in the tree
     *
     * @param key The key to search for
     * @param value Output parameter that will be set to the value if the key is found
     * @return true if the key was found, false otherwise
     *
     * Time complexity: O(log n) where n is the number of keys
     * Space complexity: O(1)
     * Exception safety: No-throw guarantee if KeyType and ValueType operations don't throw
     */
    bool search(const KeyType& key, ValueType& value) const;

    /**
     * @brief Inserts a key-value pair into the tree
     *
     * If the key already exists, its value is updated. Otherwise, a new entry is created.
     * The tree automatically splits nodes when they become full to maintain balance.
     *
     * @param key The key to insert
     * @param value The value to associate with the key
     *
     * Time complexity: O(log n) where n is the number of keys
     * Space complexity: O(log n) for the recursion stack during splits
     * Exception safety: Basic guarantee - tree remains valid but may be partially modified
     *                   if KeyType or ValueType copy/move operations throw
     */
    void insert(const KeyType& key, const ValueType& value);

    /**
     * @brief Removes a key from the tree
     *
     * If the key is found, it is removed and the tree is rebalanced if necessary
     * through redistribution or merging of nodes.
     *
     * @param key The key to remove
     * @return true if the key was found and removed, false if the key was not found
     *
     * Time complexity: O(log n) where n is the number of keys
     * Space complexity: O(log n) for the recursion stack during rebalancing
     * Exception safety: Basic guarantee - tree remains valid
     */
    bool remove(const KeyType& key);

    /**
     * @brief Performs a range query to retrieve all key-value pairs in a range
     *
     * Returns all entries with keys in the range [start, end] (inclusive).
     * Efficiently uses the linked list structure of leaf nodes.
     *
     * @param start The lower bound of the range (inclusive)
     * @param end The upper bound of the range (inclusive)
     * @return A vector of key-value pairs in the specified range, sorted by key
     *
     * Time complexity: O(log n + k) where n is the number of keys and k is the result size
     * Space complexity: O(k) for the result vector
     * Exception safety: Strong guarantee - either returns results or leaves tree unchanged
     */
    std::vector<std::pair<KeyType, ValueType>> rangeQuery(const KeyType& start,
                                                           const KeyType& end) const;

    /**
     * @brief Prints the tree structure to standard output
     *
     * Displays the tree level by level, showing keys at each node and indicating
     * whether each node is internal or a leaf.
     *
     * Time complexity: O(n) where n is the number of nodes
     * Exception safety: Basic guarantee
     */
    void print() const;

    /**
     * @brief Calculates the height of the tree
     *
     * @return The height of the tree (number of levels). Returns 0 for an empty tree.
     *
     * Time complexity: O(h) where h is the height
     * Space complexity: O(1)
     * Exception safety: No-throw guarantee
     */
    size_t height() const;

    /**
     * @brief Validates the structural integrity of the tree
     *
     * Checks that:
     * - All nodes (except root) have the correct number of keys
     * - Keys within each node are sorted
     * - All leaf nodes are at the same level
     *
     * @return true if the tree structure is valid, false otherwise
     *
     * Time complexity: O(n) where n is the number of nodes
     * Exception safety: No-throw guarantee
     */
    bool validate() const;

    /**
     * @brief Checks if the tree is empty
     *
     * @return true if the tree contains no keys, false otherwise
     *
     * Time complexity: O(1)
     * Exception safety: No-throw guarantee
     */
    bool isEmpty() const { return root == nullptr; }

    /**
     * @brief Efficiently constructs the tree from sorted data using bulk loading
     *
     * Bulk loading builds a B+ tree from pre-sorted data in O(n) time, which is
     * significantly faster than inserting elements one by one (O(n log n)).
     * The algorithm builds the tree bottom-up:
     * 1. Creates leaf nodes filled to optimal capacity
     * 2. Links leaves into a doubly-linked list
     * 3. Builds internal node levels from the bottom up
     *
     * IMPORTANT: The input data MUST be sorted in ascending order by key.
     * If the data is not sorted, the resulting tree will be invalid.
     * Duplicate keys are allowed; later duplicates will overwrite earlier ones
     * (only the last value for a duplicate key is retained).
     *
     * If the tree already contains data, it will be cleared before loading.
     *
     * @tparam InputIterator An input iterator type that dereferences to
     *         std::pair<KeyType, ValueType> or a compatible type
     * @param first Iterator to the first element in the sorted range
     * @param last Iterator to one past the last element in the sorted range
     *
     * Time complexity: O(n) where n is the number of elements
     * Space complexity: O(n) for the tree nodes
     * Exception safety: Basic guarantee - if an exception is thrown during
     *                   construction, the tree will be empty
     *
     * @code
     * // Example usage with a sorted vector:
     * std::vector<std::pair<int, std::string>> data = {{1, "a"}, {2, "b"}, {3, "c"}};
     * BPlusTree<int, std::string> tree;
     * tree.bulkLoad(data.begin(), data.end());
     * @endcode
     */
    template<typename InputIterator>
    void bulkLoad(InputIterator first, InputIterator last);

    /**
     * @brief Convenience overload for bulk loading from a vector of pairs
     *
     * @param data A vector of key-value pairs, MUST be sorted by key in ascending order
     *
     * @see bulkLoad(InputIterator, InputIterator) for details
     */
    void bulkLoad(const std::vector<std::pair<KeyType, ValueType>>& data) {
        bulkLoad(data.begin(), data.end());
    }

    /**
     * @brief Convenience overload for bulk loading from a moved vector of pairs
     *
     * This overload allows efficient bulk loading when the source vector is no longer needed.
     * Values are moved from the vector into the tree.
     *
     * @param data A vector of key-value pairs, MUST be sorted by key in ascending order
     *
     * @see bulkLoad(InputIterator, InputIterator) for details
     */
    void bulkLoad(std::vector<std::pair<KeyType, ValueType>>&& data) {
        bulkLoad(std::make_move_iterator(data.begin()), std::make_move_iterator(data.end()));
    }

    // Iterator methods

    /**
     * @brief Returns an iterator to the first element
     *
     * @return Iterator pointing to the first (smallest) key-value pair,
     *         or end() if the tree is empty
     *
     * Time complexity: O(log n) to find the first leaf
     * Exception safety: No-throw guarantee
     */
    iterator begin() {
        LeafNode<KeyType, ValueType>* first = getFirstLeaf();
        return first && first->numKeys > 0 ? iterator(first, 0) : end();
    }

    /**
     * @brief Returns a const iterator to the first element
     *
     * @return Const iterator pointing to the first (smallest) key-value pair,
     *         or end() if the tree is empty
     *
     * Time complexity: O(log n) to find the first leaf
     * Exception safety: No-throw guarantee
     */
    const_iterator begin() const {
        const LeafNode<KeyType, ValueType>* first = getFirstLeaf();
        return first && first->numKeys > 0 ? const_iterator(first, 0) : end();
    }

    /**
     * @brief Returns a const iterator to the first element
     *
     * @return Const iterator pointing to the first (smallest) key-value pair,
     *         or cend() if the tree is empty
     *
     * Time complexity: O(log n) to find the first leaf
     * Exception safety: No-throw guarantee
     */
    const_iterator cbegin() const {
        return begin();
    }

    /**
     * @brief Returns an iterator to one past the last element
     *
     * @return Iterator representing the end of the container
     *
     * Time complexity: O(log n) to find the last leaf
     * Exception safety: No-throw guarantee
     */
    iterator end() {
        if (!root) return iterator(nullptr, 0);
        LeafNode<KeyType, ValueType>* last = getLastLeaf();
        return iterator(last, last ? last->numKeys : 0);
    }

    /**
     * @brief Returns a const iterator to one past the last element
     *
     * @return Const iterator representing the end of the container
     *
     * Time complexity: O(log n) to find the last leaf
     * Exception safety: No-throw guarantee
     */
    const_iterator end() const {
        if (!root) return const_iterator(nullptr, 0);
        const LeafNode<KeyType, ValueType>* last = getLastLeaf();
        return const_iterator(last, last ? last->numKeys : 0);
    }

    /**
     * @brief Returns a const iterator to one past the last element
     *
     * @return Const iterator representing the end of the container
     *
     * Time complexity: O(1)
     * Exception safety: No-throw guarantee
     */
    const_iterator cend() const {
        return end();
    }

    /**
     * @brief Returns a reverse iterator to the first element of reversed container
     *
     * @return Reverse iterator pointing to the last (largest) key-value pair,
     *         or rend() if the tree is empty
     *
     * Time complexity: O(log n) to find the last leaf
     * Exception safety: No-throw guarantee
     */
    reverse_iterator rbegin() {
        if (!root) return reverse_iterator(nullptr, 0);
        LeafNode<KeyType, ValueType>* last = getLastLeaf();
        return last && last->numKeys > 0 ?
               reverse_iterator(last, last->numKeys - 1) :
               reverse_iterator(nullptr, 0);
    }

    /**
     * @brief Returns a const reverse iterator to the first element of reversed container
     *
     * @return Const reverse iterator pointing to the last (largest) key-value pair,
     *         or rend() if the tree is empty
     *
     * Time complexity: O(log n) to find the last leaf
     * Exception safety: No-throw guarantee
     */
    const_reverse_iterator rbegin() const {
        if (!root) return const_reverse_iterator(nullptr, 0);
        const LeafNode<KeyType, ValueType>* last = getLastLeaf();
        return last && last->numKeys > 0 ?
               const_reverse_iterator(last, last->numKeys - 1) :
               const_reverse_iterator(nullptr, 0);
    }

    /**
     * @brief Returns a const reverse iterator to the first element of reversed container
     *
     * @return Const reverse iterator pointing to the last (largest) key-value pair,
     *         or crend() if the tree is empty
     *
     * Time complexity: O(log n) to find the last leaf
     * Exception safety: No-throw guarantee
     */
    const_reverse_iterator crbegin() const {
        return rbegin();
    }

    /**
     * @brief Returns a reverse iterator to one past the last element of reversed container
     *
     * @return Reverse iterator representing the end of the reversed container
     *
     * Time complexity: O(1)
     * Exception safety: No-throw guarantee
     */
    reverse_iterator rend() {
        return reverse_iterator(nullptr, 0);
    }

    /**
     * @brief Returns a const reverse iterator to one past the last element of reversed container
     *
     * @return Const reverse iterator representing the end of the reversed container
     *
     * Time complexity: O(1)
     * Exception safety: No-throw guarantee
     */
    const_reverse_iterator rend() const {
        return const_reverse_iterator(nullptr, 0);
    }

    /**
     * @brief Returns a const reverse iterator to one past the last element of reversed container
     *
     * @return Const reverse iterator representing the end of the reversed container
     *
     * Time complexity: O(1)
     * Exception safety: No-throw guarantee
     */
    const_reverse_iterator crend() const {
        return rend();
    }
};

// Constructor
template<typename KeyType, typename ValueType>
BPlusTree<KeyType, ValueType>::BPlusTree(size_t ord)
    : root(nullptr), order(ord) {
    if (order < MIN_ORDER) {
        order = MIN_ORDER;
    }
    maxKeys = order - 1;
    minKeys = (order + 1) / 2 - 1;  // ⌈m/2⌉ - 1
}

// Destructor
template<typename KeyType, typename ValueType>
BPlusTree<KeyType, ValueType>::~BPlusTree() {
    destroyTree(root);
}

// Move constructor
template<typename KeyType, typename ValueType>
BPlusTree<KeyType, ValueType>::BPlusTree(BPlusTree&& other) noexcept
    : root(other.root), order(other.order), maxKeys(other.maxKeys), minKeys(other.minKeys) {
    other.root = nullptr;
    other.order = DEFAULT_ORDER;
    other.maxKeys = DEFAULT_ORDER - 1;
    other.minKeys = (DEFAULT_ORDER + 1) / 2 - 1;
}

// Move assignment operator
template<typename KeyType, typename ValueType>
BPlusTree<KeyType, ValueType>& BPlusTree<KeyType, ValueType>::operator=(BPlusTree&& other) noexcept {
    if (this != &other) {
        // Clean up existing tree
        destroyTree(root);

        // Move data from other
        root = other.root;
        order = other.order;
        maxKeys = other.maxKeys;
        minKeys = other.minKeys;

        // Reset other to empty state
        other.root = nullptr;
        other.order = DEFAULT_ORDER;
        other.maxKeys = DEFAULT_ORDER - 1;
        other.minKeys = (DEFAULT_ORDER + 1) / 2 - 1;
    }
    return *this;
}

template<typename KeyType, typename ValueType>
void BPlusTree<KeyType, ValueType>::destroyTree(Node<KeyType, ValueType>* node) {
    if (!node) return;

    if (node->isInternal()) {
        assert(node->isInternal() && "Expected internal node");
        InternalNode<KeyType, ValueType>* internal =
            static_cast<InternalNode<KeyType, ValueType>*>(node);
        // Only destroy actual children (numKeys + 1)
        size_t numChildren = node->numKeys + 1;
        for (size_t i = 0; i < numChildren; i++) {
            if (internal->children[i]) {
                destroyTree(internal->children[i]);
            }
        }
    }
    delete node;
}

// Search operation
template<typename KeyType, typename ValueType>
bool BPlusTree<KeyType, ValueType>::search(const KeyType& key, ValueType& value) const {
    if (!root) return false;

    const LeafNode<KeyType, ValueType>* leaf = findLeaf(key);
    return leaf->findValue(key, value);
}

template<typename KeyType, typename ValueType>
LeafNode<KeyType, ValueType>* BPlusTree<KeyType, ValueType>::findLeaf(const KeyType& key) {
    Node<KeyType, ValueType>* current = root;

    while (current && current->isInternal()) {
        assert(current->isInternal() && "Expected internal node");
        InternalNode<KeyType, ValueType>* internal =
            static_cast<InternalNode<KeyType, ValueType>*>(current);
        size_t index = internal->findChildIndex(key);
        current = internal->children[index];
    }

    assert(current == nullptr || current->isLeaf() && "Expected leaf node or null");
    return static_cast<LeafNode<KeyType, ValueType>*>(current);
}

template<typename KeyType, typename ValueType>
const LeafNode<KeyType, ValueType>* BPlusTree<KeyType, ValueType>::findLeaf(const KeyType& key) const {
    const Node<KeyType, ValueType>* current = root;

    while (current && current->isInternal()) {
        assert(current->isInternal() && "Expected internal node");
        const InternalNode<KeyType, ValueType>* internal =
            static_cast<const InternalNode<KeyType, ValueType>*>(current);
        size_t index = internal->findChildIndex(key);
        current = internal->children[index];
    }

    assert(current == nullptr || current->isLeaf() && "Expected leaf node or null");
    return static_cast<const LeafNode<KeyType, ValueType>*>(current);
}

// Insert operation
template<typename KeyType, typename ValueType>
void BPlusTree<KeyType, ValueType>::insert(const KeyType& key, const ValueType& value) {
    // Empty tree case
    if (!root) {
        root = new LeafNode<KeyType, ValueType>(maxKeys);
        assert(root->isLeaf() && "Root should be a leaf node");
        LeafNode<KeyType, ValueType>* leaf = static_cast<LeafNode<KeyType, ValueType>*>(root);
        leaf->insertAt(0, key, value);
        return;
    }

    // Find the appropriate leaf node
    LeafNode<KeyType, ValueType>* leaf = findLeaf(key);

    // Check for duplicate key
    size_t pos = leaf->findKeyPosition(key);
    if (pos < leaf->numKeys && leaf->keys[pos] == key) {
        // Update existing value
        leaf->values[pos] = value;
        return;
    }

    // Insert into leaf
    leaf->insertAt(pos, key, value);

    // Split if necessary
    if (leaf->isFull()) {
        splitLeaf(leaf);
    }
}

template<typename KeyType, typename ValueType>
void BPlusTree<KeyType, ValueType>::splitLeaf(LeafNode<KeyType, ValueType>* leaf) {
    // Create new leaf node
    LeafNode<KeyType, ValueType>* newLeaf = nullptr;

    try {
        newLeaf = new LeafNode<KeyType, ValueType>(maxKeys);

        // Calculate split point
        size_t splitPoint = (maxKeys + 1) / 2;

        // Move second half of keys and values to new leaf using direct indexing
        size_t newLeafIndex = 0;
        for (size_t i = splitPoint; i < leaf->numKeys; i++) {
            newLeaf->keys[newLeafIndex] = std::move(leaf->keys[i]);
            newLeaf->values[newLeafIndex] = std::move(leaf->values[i]);
            newLeafIndex++;
        }
        newLeaf->numKeys = newLeafIndex;

        // Adjust original leaf - just update count, no need to resize
        leaf->numKeys = splitPoint;

        // Update linked list
        newLeaf->next = leaf->next;
        newLeaf->prev = leaf;
        if (leaf->next) {
            leaf->next->prev = newLeaf;
        }
        leaf->next = newLeaf;

        // Insert into parent (promote the first key of new leaf)
        KeyType promoteKey = newLeaf->keys[0];
        insertIntoParent(leaf, promoteKey, newLeaf);
    } catch (...) {
        // If an exception occurs, clean up the new leaf
        delete newLeaf;
        throw; // Re-throw the exception
    }
}

template<typename KeyType, typename ValueType>
void BPlusTree<KeyType, ValueType>::splitInternal(InternalNode<KeyType, ValueType>* node) {
    // Create new internal node
    InternalNode<KeyType, ValueType>* newNode = nullptr;

    // Calculate split point and save original children count (needed for exception handling)
    size_t splitPoint = (maxKeys + 1) / 2;
    size_t numOriginalChildren = node->numKeys + 1;

    try {
        newNode = new InternalNode<KeyType, ValueType>(maxKeys);

        // Key to promote to parent
        KeyType promoteKey = node->keys[splitPoint];

        // Move second half of keys to new node using direct indexing
        size_t newNodeKeyIndex = 0;
        for (size_t i = splitPoint + 1; i < node->numKeys; i++) {
            newNode->keys[newNodeKeyIndex] = std::move(node->keys[i]);
            newNodeKeyIndex++;
        }
        newNode->numKeys = newNodeKeyIndex;

        // Move children using direct indexing
        size_t newNodeChildIndex = 0;
        for (size_t i = splitPoint + 1; i < numOriginalChildren; i++) {
            newNode->children[newNodeChildIndex] = node->children[i];
            if (node->children[i]) {
                node->children[i]->parent = newNode;
            }
            node->children[i] = nullptr;  // Nullify after moving to prevent double-deletion
            newNodeChildIndex++;
        }

        // Adjust original node - just update count
        node->numKeys = splitPoint;

        // Insert into parent
        insertIntoParent(node, promoteKey, newNode);
    } catch (...) {
        // If an exception occurs, clean up the new node
        // Note: children have already been transferred and nullified, so we need to restore them
        if (newNode) {
            // Restore children back to original node
            size_t numNewChildren = newNode->numKeys + 1;
            size_t originalIndex = splitPoint + 1;
            for (size_t i = 0; i < numNewChildren; i++) {
                if (newNode->children[i]) {
                    newNode->children[i]->parent = node;
                    node->children[originalIndex + i] = newNode->children[i];  // Restore pointer
                }
            }
            // Restore original numKeys
            node->numKeys = numOriginalChildren - 1;
            delete newNode;
        }
        throw; // Re-throw the exception
    }
}

template<typename KeyType, typename ValueType>
void BPlusTree<KeyType, ValueType>::insertIntoParent(
    Node<KeyType, ValueType>* left,
    const KeyType& key,
    Node<KeyType, ValueType>* right) {

    // If left is root, create new root
    if (left->parent == nullptr) {
        InternalNode<KeyType, ValueType>* newRoot =
            new InternalNode<KeyType, ValueType>(maxKeys);
        newRoot->keys[0] = key;
        newRoot->numKeys = 1;
        newRoot->children[0] = left;
        newRoot->children[1] = right;
        left->parent = newRoot;
        right->parent = newRoot;
        root = newRoot;
        return;
    }

    // Insert into existing parent
    InternalNode<KeyType, ValueType>* parent =
        static_cast<InternalNode<KeyType, ValueType>*>(left->parent);

    size_t pos = parent->findKeyPosition(key);
    parent->insertKeyAt(pos, key);
    parent->insertChildAt(pos + 1, right);

    // Split parent if necessary
    if (parent->isFull()) {
        splitInternal(parent);
    }
}

// Delete operation
template<typename KeyType, typename ValueType>
bool BPlusTree<KeyType, ValueType>::remove(const KeyType& key) {
    if (!root) return false;

    LeafNode<KeyType, ValueType>* leaf = findLeaf(key);
    if (!leaf) return false;  // Leaf not found (tree structure issue)

    // Find the key
    bool found = false;
    size_t pos = 0;
    for (size_t i = 0; i < leaf->numKeys; i++) {
        if (leaf->keys[i] == key) {
            pos = i;
            found = true;
            break;
        }
    }

    if (!found) return false;  // Key not found

    // Remove the key
    leaf->removeAt(pos);

    // Handle underflow
    if (leaf == root) {
        // Root can have fewer keys
        if (leaf->numKeys == 0) {
            delete root;
            root = nullptr;
        }
        return true;
    }

    if (leaf->isUnderflow(minKeys)) {
        deleteEntry(leaf);
    }

    return true;
}

template<typename KeyType, typename ValueType>
void BPlusTree<KeyType, ValueType>::deleteEntry(Node<KeyType, ValueType>* node) {
    if (node == root) {
        if (node->numKeys == 0) {
            if (node->isInternal()) {
                assert(node->isInternal() && "Expected internal node");
                InternalNode<KeyType, ValueType>* internal =
                    static_cast<InternalNode<KeyType, ValueType>*>(node);
                if (!internal->children.empty()) {
                    root = internal->children[0];
                    root->parent = nullptr;
                } else {
                    root = nullptr;
                }
            } else {
                root = nullptr;
            }
            delete node;
        }
        return;
    }

    assert(node->parent && "Non-root node must have a parent");
    assert(node->parent->isInternal() && "Parent must be an internal node");
    InternalNode<KeyType, ValueType>* parent =
        static_cast<InternalNode<KeyType, ValueType>*>(node->parent);
    int nodeIndex = getNodeIndex(node);

    // Verify node was found in parent's children
    if (nodeIndex == -1) {
        std::cerr << "Error: Node not found in parent's children list" << std::endl;
        return;
    }

    // Try to borrow from sibling
    if (nodeIndex > 0) {
        Node<KeyType, ValueType>* leftSibling = parent->children[nodeIndex - 1];
        // Add assertion to ensure sibling is valid
        assert(leftSibling && "Left sibling should not be null");
        if (leftSibling->numKeys > minKeys) {
            redistributeNodes(node, leftSibling, nodeIndex - 1, true);
            return;
        }
    }

    // Check if there's a right sibling (numChildren = numKeys + 1)
    if (static_cast<size_t>(nodeIndex) < parent->numKeys) {
        Node<KeyType, ValueType>* rightSibling = parent->children[nodeIndex + 1];
        // Add assertion to ensure sibling is valid
        assert(rightSibling && "Right sibling should not be null");
        if (rightSibling->numKeys > minKeys) {
            redistributeNodes(node, rightSibling, nodeIndex, false);
            return;
        }
    }

    // Merge with sibling
    if (nodeIndex > 0) {
        Node<KeyType, ValueType>* leftSibling = parent->children[nodeIndex - 1];
        assert(leftSibling && "Left sibling should not be null");
        mergeNodes(leftSibling, node, nodeIndex - 1, true);
    } else {
        Node<KeyType, ValueType>* rightSibling = parent->children[nodeIndex + 1];
        assert(rightSibling && "Right sibling should not be null");
        mergeNodes(node, rightSibling, nodeIndex, false);
    }
}

/**
 * @brief Merges two sibling nodes when redistribution is not possible
 *
 * This function combines all keys from the right node into the left node.
 * For internal nodes, it also pulls down the separator key from the parent.
 * After merging, the right node is deleted and the parent is updated.
 *
 * Algorithm steps:
 * 1. For leaf nodes:
 *    - Move all keys/values from right to left
 *    - Update the linked list pointers
 * 2. For internal nodes:
 *    - Pull down separator key from parent
 *    - Move all keys from right to left
 *    - Move all child pointers and update their parent references
 * 3. Update parent by removing the separator key and right child pointer
 * 4. Recursively handle parent underflow if necessary
 *
 * @param left The left sibling node (will contain merged data)
 * @param right The right sibling node (will be deleted)
 * @param parentIndex Index of the separator key in the parent
 * @param isLeftSibling Unused parameter (kept for consistency with redistributeNodes)
 */
template<typename KeyType, typename ValueType>
void BPlusTree<KeyType, ValueType>::mergeNodes(
    Node<KeyType, ValueType>* left,
    Node<KeyType, ValueType>* right,
    int parentIndex,
    bool /* isLeftSibling */) {

    if (left->isLeaf()) {
        assert(left->isLeaf() && right->isLeaf() && "Both nodes must be leaves");
        LeafNode<KeyType, ValueType>* leftLeaf =
            static_cast<LeafNode<KeyType, ValueType>*>(left);
        LeafNode<KeyType, ValueType>* rightLeaf =
            static_cast<LeafNode<KeyType, ValueType>*>(right);

        // Step 1: Move all keys/values from right leaf to left leaf
        // Use direct array indexing for better performance than vector operations
        size_t leftIndex = leftLeaf->numKeys;
        for (size_t i = 0; i < rightLeaf->numKeys; i++) {
            leftLeaf->keys[leftIndex] = std::move(rightLeaf->keys[i]);
            leftLeaf->values[leftIndex] = std::move(rightLeaf->values[i]);
            leftIndex++;
        }
        leftLeaf->numKeys = leftIndex;

        // Step 2: Update the doubly-linked list to remove right leaf
        // This maintains sequential access capability across leaf nodes
        leftLeaf->next = rightLeaf->next;
        if (rightLeaf->next) {
            rightLeaf->next->prev = leftLeaf;
        }

        // Step 3: Delete the now-empty right leaf
        delete rightLeaf;
    } else {
        assert(left->isInternal() && right->isInternal() && "Both nodes must be internal");
        InternalNode<KeyType, ValueType>* leftInternal =
            static_cast<InternalNode<KeyType, ValueType>*>(left);
        InternalNode<KeyType, ValueType>* rightInternal =
            static_cast<InternalNode<KeyType, ValueType>*>(right);

        // Step 1: Save the original number of children in left node
        // This is needed to know where to insert the children from right node
        // (must be saved before modifying numKeys)
        size_t originalLeftChildren = leftInternal->numKeys + 1;

        // Step 2: Pull down the separator key from parent
        // In B+ trees, internal node merging requires bringing down the separator key
        // Example: Parent has key K separating left [A,B] and right [D,E]
        //          After merge: left becomes [A,B,K,D,E]
        assert(left->parent && left->parent->isInternal() && "Parent must be internal");
        InternalNode<KeyType, ValueType>* parent =
            static_cast<InternalNode<KeyType, ValueType>*>(left->parent);
        leftInternal->keys[leftInternal->numKeys] = parent->keys[parentIndex];
        leftInternal->numKeys++;

        // Step 3: Move all keys from right to left using direct array indexing
        size_t leftKeyIndex = leftInternal->numKeys;
        for (size_t i = 0; i < rightInternal->numKeys; i++) {
            leftInternal->keys[leftKeyIndex] = std::move(rightInternal->keys[i]);
            leftKeyIndex++;
        }
        leftInternal->numKeys = leftKeyIndex;

        // Step 4: Move all child pointers from right to left
        // CRITICAL: Children from right must be appended after left's existing children
        // Use originalLeftChildren (not numKeys+1) because numKeys has changed
        size_t leftChildIndex = originalLeftChildren;
        size_t numRightChildren = rightInternal->numKeys + 1;
        for (size_t i = 0; i < numRightChildren; i++) {
            leftInternal->children[leftChildIndex] = rightInternal->children[i];
            // Update parent pointers to maintain tree structure
            if (rightInternal->children[i]) {
                rightInternal->children[i]->parent = leftInternal;
            }
            leftChildIndex++;
        }

        // Step 5: Delete the now-empty right internal node
        delete rightInternal;
    }

    // Step 6: Remove separator key and right child pointer from parent
    // CRITICAL ORDER: Remove child BEFORE removing key!
    // removeChildAt() uses numKeys to determine valid children range,
    // so we must remove the child first to avoid incorrect bounds
    assert(left->parent && left->parent->isInternal() && "Parent must be internal");
    InternalNode<KeyType, ValueType>* parent =
        static_cast<InternalNode<KeyType, ValueType>*>(left->parent);
    parent->removeChildAt(parentIndex + 1);  // Remove right child pointer
    parent->removeKeyAt(parentIndex);         // Remove separator key

    // Step 7: Recursively handle parent underflow if it now has too few keys
    // This may cascade up the tree, potentially decreasing tree height
    if (parent->isUnderflow(minKeys)) {
        deleteEntry(parent);
    }
}

/**
 * @brief Redistributes keys between a node and its sibling to fix underflow
 *
 * This function borrows a key from a sibling node that has more than the minimum
 * number of keys. This is preferred over merging because it doesn't reduce the
 * number of nodes in the tree. The parent's separator key is updated accordingly.
 *
 * Algorithm varies based on node type and sibling position:
 *
 * For LEAF nodes borrowing from LEFT sibling:
 *   1. Shift all keys/values in node one position to the right
 *   2. Move rightmost key/value from sibling to leftmost position in node
 *   3. Update parent separator to be node's new first key
 *
 * For LEAF nodes borrowing from RIGHT sibling:
 *   1. Move leftmost key/value from sibling to rightmost position in node
 *   2. Shift all keys/values in sibling one position to the left
 *   3. Update parent separator to be sibling's new first key
 *
 * For INTERNAL nodes, the process includes rotating the parent separator key.
 *
 * @param node The underflow node that needs a key
 * @param sibling The sibling node with extra keys
 * @param parentIndex Index of the separator key in the parent
 * @param isLeftSibling True if borrowing from left sibling, false for right
 */
template<typename KeyType, typename ValueType>
void BPlusTree<KeyType, ValueType>::redistributeNodes(
    Node<KeyType, ValueType>* node,
    Node<KeyType, ValueType>* sibling,
    int parentIndex,
    bool isLeftSibling) {

    assert(node->parent && node->parent->isInternal() && "Parent must be internal");
    InternalNode<KeyType, ValueType>* parent =
        static_cast<InternalNode<KeyType, ValueType>*>(node->parent);

    if (node->isLeaf()) {
        assert(node->isLeaf() && sibling->isLeaf() && "Both nodes must be leaves");
        LeafNode<KeyType, ValueType>* leaf = static_cast<LeafNode<KeyType, ValueType>*>(node);
        LeafNode<KeyType, ValueType>* siblingLeaf =
            static_cast<LeafNode<KeyType, ValueType>*>(sibling);

        if (isLeftSibling) {
            // Borrowing from left sibling
            // Example: left [10,20,30], node [50] -> left [10,20], node [30,50]

            // Step 1: Shift all entries in node one position to the right
            // to make room for the borrowed key at index 0
            for (size_t i = leaf->numKeys; i > 0; --i) {
                leaf->keys[i] = std::move(leaf->keys[i - 1]);
                leaf->values[i] = std::move(leaf->values[i - 1]);
            }

            // Step 2: Move the rightmost entry from left sibling to node's first position
            leaf->keys[0] = std::move(siblingLeaf->keys[siblingLeaf->numKeys - 1]);
            leaf->values[0] = std::move(siblingLeaf->values[siblingLeaf->numKeys - 1]);
            leaf->numKeys++;
            siblingLeaf->numKeys--;

            // Step 3: Update parent separator key to be node's new first key
            // This maintains the invariant that parent key equals first key of right child
            parent->keys[parentIndex] = leaf->keys[0];
        } else {
            // Borrowing from right sibling
            // Example: node [10], right [20,30,40] -> node [10,20], right [30,40]

            // Step 1: Append the leftmost entry from right sibling to end of node
            leaf->keys[leaf->numKeys] = std::move(siblingLeaf->keys[0]);
            leaf->values[leaf->numKeys] = std::move(siblingLeaf->values[0]);
            leaf->numKeys++;

            // Step 2: Shift all entries in right sibling one position to the left
            // to fill the gap left by the borrowed key
            for (size_t i = 0; i < siblingLeaf->numKeys - 1; ++i) {
                siblingLeaf->keys[i] = std::move(siblingLeaf->keys[i + 1]);
                siblingLeaf->values[i] = std::move(siblingLeaf->values[i + 1]);
            }
            siblingLeaf->numKeys--;

            // Step 3: Update parent separator key to be sibling's new first key
            parent->keys[parentIndex] = siblingLeaf->keys[0];
        }
    } else {
        assert(node->isInternal() && sibling->isInternal() && "Both nodes must be internal");
        InternalNode<KeyType, ValueType>* internal =
            static_cast<InternalNode<KeyType, ValueType>*>(node);
        InternalNode<KeyType, ValueType>* siblingInternal =
            static_cast<InternalNode<KeyType, ValueType>*>(sibling);

        if (isLeftSibling) {
            // Borrowing from left sibling (internal node)
            // This involves a rotation through the parent
            // Example: parent separator K, left [...,A], node [C,...]
            //          After: left [...], parent separator A, node [K,C,...]

            // Step 1: Shift all keys in node one position to the right
            for (size_t i = internal->numKeys; i > 0; --i) {
                internal->keys[i] = std::move(internal->keys[i - 1]);
            }
            // Step 2: Pull down the parent separator key to node's first position
            internal->keys[0] = parent->keys[parentIndex];
            internal->numKeys++;

            // Step 3: Shift all child pointers in node one position to the right
            size_t numChildren = internal->numKeys;
            for (size_t i = numChildren; i > 0; --i) {
                internal->children[i] = internal->children[i - 1];
            }
            // Step 4: Move the rightmost child from left sibling to node's first child
            internal->children[0] = siblingInternal->children[siblingInternal->numKeys];
            internal->children[0]->parent = internal;  // Update parent reference

            // Step 5: Push up the rightmost key from left sibling to parent
            parent->keys[parentIndex] = siblingInternal->keys[siblingInternal->numKeys - 1];
            siblingInternal->numKeys--;
        } else {
            // Borrowing from right sibling (internal node)
            // Example: parent separator K, node [...,A], right [C,...]
            //          After: node [...,A,K], parent separator C, right [...]

            // Step 1: Pull down the parent separator key to end of node
            internal->keys[internal->numKeys] = parent->keys[parentIndex];
            internal->numKeys++;

            // Step 2: Move the leftmost child from right sibling to end of node
            internal->children[internal->numKeys] = siblingInternal->children[0];
            siblingInternal->children[0]->parent = internal;  // Update parent reference

            // Step 3: Push up the leftmost key from right sibling to parent
            parent->keys[parentIndex] = siblingInternal->keys[0];

            // Step 4: Shift all keys in right sibling one position to the left
            for (size_t i = 0; i < siblingInternal->numKeys - 1; ++i) {
                siblingInternal->keys[i] = std::move(siblingInternal->keys[i + 1]);
            }
            // Step 5: Shift all child pointers in right sibling one position to the left
            size_t numSiblingChildren = siblingInternal->numKeys + 1;
            for (size_t i = 0; i < numSiblingChildren - 1; ++i) {
                siblingInternal->children[i] = siblingInternal->children[i + 1];
            }
            siblingInternal->numKeys--;
        }
    }
}

template<typename KeyType, typename ValueType>
int BPlusTree<KeyType, ValueType>::getNodeIndex(Node<KeyType, ValueType>* node) {
    InternalNode<KeyType, ValueType>* parent =
        static_cast<InternalNode<KeyType, ValueType>*>(node->parent);

    // Number of children = numKeys + 1
    size_t numChildren = parent->numKeys + 1;
    for (size_t i = 0; i < numChildren; i++) {
        if (parent->children[i] == node) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// Range query
template<typename KeyType, typename ValueType>
std::vector<std::pair<KeyType, ValueType>> BPlusTree<KeyType, ValueType>::rangeQuery(
    const KeyType& start,
    const KeyType& end) const {

    std::vector<std::pair<KeyType, ValueType>> result;

    if (!root) return result;

    // Find starting leaf
    const LeafNode<KeyType, ValueType>* leaf = findLeaf(start);

    // Traverse leaves and collect results
    while (leaf) {
        for (size_t i = 0; i < leaf->numKeys; i++) {
            if (leaf->keys[i] >= start && leaf->keys[i] <= end) {
                result.emplace_back(leaf->keys[i], leaf->values[i]);
            } else if (leaf->keys[i] > end) {
                return result;
            }
        }
        leaf = leaf->next;
    }

    return result;
}

// Utility functions
template<typename KeyType, typename ValueType>
void BPlusTree<KeyType, ValueType>::print() const {
    if (!root) {
        std::cout << "Empty tree" << std::endl;
        return;
    }
    printNode(root, 0);
}

template<typename KeyType, typename ValueType>
void BPlusTree<KeyType, ValueType>::printNode(const Node<KeyType, ValueType>* node, int level) const {
    if (!node) return;

    std::cout << "Level " << level << ": [";
    for (size_t i = 0; i < node->numKeys; i++) {
        if (i > 0) std::cout << ", ";
        std::cout << node->keys[i];
    }
    std::cout << "]";

    if (node->isLeaf()) {
        std::cout << " (Leaf)" << std::endl;
    } else {
        std::cout << " (Internal)" << std::endl;
        assert(node->isInternal() && "Expected internal node");
        const InternalNode<KeyType, ValueType>* internal =
            static_cast<const InternalNode<KeyType, ValueType>*>(node);
        // Only print actual children (numKeys + 1)
        size_t numChildren = node->numKeys + 1;
        for (size_t i = 0; i < numChildren; i++) {
            printNode(internal->children[i], level + 1);
        }
    }
}

template<typename KeyType, typename ValueType>
size_t BPlusTree<KeyType, ValueType>::height() const {
    if (!root) return 0;

    size_t h = 1;
    const Node<KeyType, ValueType>* current = root;
    while (current->isInternal()) {
        assert(current->isInternal() && "Expected internal node");
        const InternalNode<KeyType, ValueType>* internal =
            static_cast<const InternalNode<KeyType, ValueType>*>(current);
        current = internal->children[0];
        h++;
    }
    return h;
}

template<typename KeyType, typename ValueType>
bool BPlusTree<KeyType, ValueType>::validate() const {
    if (!root) return true;

    int leafLevel = -1;
    return validateNode(root, 0, leafLevel);
}

template<typename KeyType, typename ValueType>
bool BPlusTree<KeyType, ValueType>::validateNode(const Node<KeyType, ValueType>* node,
                                                   int level, int& leafLevel) const {
    if (!node) return true;

    // Check key count bounds
    if (node != root) {
        if (node->numKeys < minKeys || node->numKeys > maxKeys) {
            std::cerr << "Invalid key count at level " << level << std::endl;
            return false;
        }
    }

    // Check keys are sorted
    for (size_t i = 1; i < node->numKeys; i++) {
        if (node->keys[i - 1] >= node->keys[i]) {
            std::cerr << "Keys not sorted at level " << level << std::endl;
            return false;
        }
    }

    if (node->isLeaf()) {
        // All leaves should be at same level
        if (leafLevel == -1) {
            leafLevel = level;
        } else if (leafLevel != level) {
            std::cerr << "Leaves at different levels" << std::endl;
            return false;
        }
    } else {
        assert(node->isInternal() && "Expected internal node");
        const InternalNode<KeyType, ValueType>* internal =
            static_cast<const InternalNode<KeyType, ValueType>*>(node);

        // Validate children - only check actual children (numKeys + 1)
        size_t numChildren = node->numKeys + 1;
        for (size_t i = 0; i < numChildren; i++) {
            if (!validateNode(internal->children[i], level + 1, leafLevel)) {
                return false;
            }
        }
    }

    return true;
}

// Helper methods for iterators
template<typename KeyType, typename ValueType>
LeafNode<KeyType, ValueType>* BPlusTree<KeyType, ValueType>::getFirstLeaf() {
    if (!root) return nullptr;

    Node<KeyType, ValueType>* current = root;
    while (current->isInternal()) {
        assert(current->isInternal() && "Expected internal node");
        InternalNode<KeyType, ValueType>* internal =
            static_cast<InternalNode<KeyType, ValueType>*>(current);
        current = internal->children[0];
    }

    assert(current->isLeaf() && "Expected leaf node");
    return static_cast<LeafNode<KeyType, ValueType>*>(current);
}

template<typename KeyType, typename ValueType>
const LeafNode<KeyType, ValueType>* BPlusTree<KeyType, ValueType>::getFirstLeaf() const {
    if (!root) return nullptr;

    const Node<KeyType, ValueType>* current = root;
    while (current->isInternal()) {
        assert(current->isInternal() && "Expected internal node");
        const InternalNode<KeyType, ValueType>* internal =
            static_cast<const InternalNode<KeyType, ValueType>*>(current);
        current = internal->children[0];
    }

    assert(current->isLeaf() && "Expected leaf node");
    return static_cast<const LeafNode<KeyType, ValueType>*>(current);
}

template<typename KeyType, typename ValueType>
LeafNode<KeyType, ValueType>* BPlusTree<KeyType, ValueType>::getLastLeaf() {
    if (!root) return nullptr;

    Node<KeyType, ValueType>* current = root;
    while (current->isInternal()) {
        assert(current->isInternal() && "Expected internal node");
        InternalNode<KeyType, ValueType>* internal =
            static_cast<InternalNode<KeyType, ValueType>*>(current);
        // Last child is at index numKeys
        current = internal->children[current->numKeys];
    }

    assert(current->isLeaf() && "Expected leaf node");
    return static_cast<LeafNode<KeyType, ValueType>*>(current);
}

template<typename KeyType, typename ValueType>
const LeafNode<KeyType, ValueType>* BPlusTree<KeyType, ValueType>::getLastLeaf() const {
    if (!root) return nullptr;

    const Node<KeyType, ValueType>* current = root;
    while (current->isInternal()) {
        assert(current->isInternal() && "Expected internal node");
        const InternalNode<KeyType, ValueType>* internal =
            static_cast<const InternalNode<KeyType, ValueType>*>(current);
        // Last child is at index numKeys
        current = internal->children[current->numKeys];
    }

    assert(current->isLeaf() && "Expected leaf node");
    return static_cast<const LeafNode<KeyType, ValueType>*>(current);
}

// Bulk loading implementation
template<typename KeyType, typename ValueType>
template<typename InputIterator>
void BPlusTree<KeyType, ValueType>::bulkLoad(InputIterator first, InputIterator last) {
    // Clear existing tree if any
    if (root) {
        destroyTree(root);
        root = nullptr;
    }

    // Handle empty input
    if (first == last) {
        return;
    }

    // Step 1: Collect all data into a temporary buffer (handling duplicates)
    std::vector<std::pair<KeyType, ValueType>> buffer;
    for (auto it = first; it != last; ++it) {
        // Handle duplicate keys: if current key equals the last key, overwrite
        if (!buffer.empty() && buffer.back().first == it->first) {
            buffer.back().second = it->second;
        } else {
            buffer.emplace_back(it->first, it->second);
        }
    }

    // Handle empty input
    if (buffer.empty()) {
        return;
    }

    // Step 2: Calculate optimal leaf distribution
    size_t totalElements = buffer.size();
    size_t numLeaves;

    if (totalElements <= maxKeys) {
        // Fits in a single leaf (which will be the root)
        numLeaves = 1;
    } else {
        // Calculate number of leaves needed
        // Each leaf can have at most maxKeys and at least minKeys elements
        numLeaves = (totalElements + maxKeys - 1) / maxKeys;  // Minimum leaves needed

        // Ensure each leaf has at least minKeys elements
        size_t maxPossibleLeaves = totalElements / minKeys;
        if (numLeaves > maxPossibleLeaves && maxPossibleLeaves > 0) {
            numLeaves = maxPossibleLeaves;
        }
    }

    // Step 3: Build leaf nodes with evenly distributed elements
    std::vector<LeafNode<KeyType, ValueType>*> leaves;
    LeafNode<KeyType, ValueType>* prevLeaf = nullptr;

    try {
        size_t elementIndex = 0;
        for (size_t leafIdx = 0; leafIdx < numLeaves; ++leafIdx) {
            LeafNode<KeyType, ValueType>* newLeaf =
                new LeafNode<KeyType, ValueType>(maxKeys);
            leaves.push_back(newLeaf);

            // Link to previous leaf
            if (prevLeaf) {
                prevLeaf->next = newLeaf;
                newLeaf->prev = prevLeaf;
            }

            // Calculate how many elements this leaf should get
            size_t remainingLeaves = numLeaves - leafIdx;
            size_t remainingElements = totalElements - elementIndex;
            size_t elementsForThis = (remainingElements + remainingLeaves - 1) / remainingLeaves;

            // Don't exceed maxKeys
            if (elementsForThis > maxKeys) {
                elementsForThis = maxKeys;
            }

            // Fill the leaf
            for (size_t e = 0; e < elementsForThis && elementIndex < totalElements; ++e) {
                newLeaf->keys[newLeaf->numKeys] = std::move(buffer[elementIndex].first);
                newLeaf->values[newLeaf->numKeys] = std::move(buffer[elementIndex].second);
                newLeaf->numKeys++;
                elementIndex++;
            }

            prevLeaf = newLeaf;
        }

        // If we only have one leaf, it becomes the root
        if (leaves.size() == 1) {
            root = leaves[0];
            return;
        }

        // Step 4: Build internal node levels from bottom up
        std::vector<Node<KeyType, ValueType>*> currentLevel(leaves.begin(), leaves.end());

        // Helper lambda to get the leftmost key in a subtree (used for separator keys)
        auto getLeftmostKey = [](Node<KeyType, ValueType>* node) -> KeyType {
            while (node->isInternal()) {
                InternalNode<KeyType, ValueType>* internal =
                    static_cast<InternalNode<KeyType, ValueType>*>(node);
                node = internal->children[0];
            }
            return node->keys[0];
        };

        while (currentLevel.size() > 1) {
            std::vector<Node<KeyType, ValueType>*> nextLevel;

            // Calculate how many children can fit in each internal node
            // Each internal node can have at most (maxKeys + 1) children
            size_t maxChildren = maxKeys + 1;
            size_t minChildren = minKeys + 1;  // Minimum children for non-root internal nodes

            // Calculate the number of internal nodes needed
            size_t numChildren = currentLevel.size();
            size_t numInternalNodes = (numChildren + maxChildren - 1) / maxChildren;

            // Ensure each internal node gets at least minChildren
            // With k nodes and n children, the minimum any node gets is floor(n/k)
            // We need floor(n/k) >= minChildren, so k <= floor(n/minChildren)
            size_t maxPossibleNodes = numChildren / minChildren;
            if (maxPossibleNodes == 0) {
                maxPossibleNodes = 1;  // At least 1 node (will be root)
            }
            if (numInternalNodes > maxPossibleNodes) {
                numInternalNodes = maxPossibleNodes;
            }

            // Build internal nodes with distributed children
            size_t childIndex = 0;
            for (size_t nodeIdx = 0; nodeIdx < numInternalNodes; ++nodeIdx) {
                InternalNode<KeyType, ValueType>* newInternal =
                    new InternalNode<KeyType, ValueType>(maxKeys);
                nextLevel.push_back(newInternal);

                // Calculate how many children this node should get
                size_t remainingNodes = numInternalNodes - nodeIdx;
                size_t remainingChildren = numChildren - childIndex;
                size_t childrenForThis = (remainingChildren + remainingNodes - 1) / remainingNodes;

                // Don't exceed maxChildren
                if (childrenForThis > maxChildren) {
                    childrenForThis = maxChildren;
                }

                // Assign children to this internal node
                for (size_t c = 0; c < childrenForThis && childIndex < numChildren; ++c) {
                    if (c == 0) {
                        // First child - no separator key needed
                        newInternal->children[0] = currentLevel[childIndex];
                        currentLevel[childIndex]->parent = newInternal;
                    } else {
                        // Add separator key and child
                        KeyType separatorKey = getLeftmostKey(currentLevel[childIndex]);
                        newInternal->keys[newInternal->numKeys] = separatorKey;
                        newInternal->numKeys++;
                        newInternal->children[newInternal->numKeys] = currentLevel[childIndex];
                        currentLevel[childIndex]->parent = newInternal;
                    }
                    childIndex++;
                }
            }

            currentLevel = std::move(nextLevel);
        }

        // The last remaining node is the root
        root = currentLevel[0];

    } catch (...) {
        // Clean up all allocated nodes on exception
        for (auto* leaf : leaves) {
            delete leaf;
        }
        root = nullptr;
        throw;
    }
}

} // namespace bptree

#endif // BPLUSTREE_H
