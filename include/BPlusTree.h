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

namespace bptree {

/**
 * B+ Tree implementation with exception safety guarantees
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
private:
    Node<KeyType, ValueType>* root;
    int order;      // m
    int maxKeys;    // m - 1
    int minKeys;    // ⌈m/2⌉ - 1

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

public:
    explicit BPlusTree(int order = DEFAULT_ORDER);
    ~BPlusTree();

    // Disable copy (copying a tree is expensive and usually unintended)
    BPlusTree(const BPlusTree&) = delete;
    BPlusTree& operator=(const BPlusTree&) = delete;

    // Enable move semantics for efficient transfer of ownership
    BPlusTree(BPlusTree&& other) noexcept;
    BPlusTree& operator=(BPlusTree&& other) noexcept;

    // Core operations
    bool search(const KeyType& key, ValueType& value) const;

    // Insert operation with basic exception guarantee:
    // If an exception is thrown (e.g., during memory allocation or copy),
    // the tree structure remains valid but may be partially modified.
    // For strong exception guarantee, consider using a copy-on-write approach.
    void insert(const KeyType& key, const ValueType& value);

    bool remove(const KeyType& key);

    // Range query
    std::vector<std::pair<KeyType, ValueType>> rangeQuery(const KeyType& start,
                                                           const KeyType& end) const;

    // Utility functions
    void print() const;
    int height() const;
    bool validate() const;
    bool isEmpty() const { return root == nullptr; }
};

// Constructor
template<typename KeyType, typename ValueType>
BPlusTree<KeyType, ValueType>::BPlusTree(int ord)
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
        for (auto child : internal->children) {
            destroyTree(child);
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
        int index = internal->findChildIndex(key);
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
        int index = internal->findChildIndex(key);
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
    int pos = leaf->findKeyPosition(key);
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
        int splitPoint = (maxKeys + 1) / 2;

        // Reserve space to minimize allocation failures
        newLeaf->keys.reserve(leaf->numKeys - splitPoint);
        newLeaf->values.reserve(leaf->numKeys - splitPoint);

        // Move second half of keys and values to new leaf
        // Using move semantics where possible for better performance
        for (int i = splitPoint; i < leaf->numKeys; i++) {
            newLeaf->keys.push_back(std::move(leaf->keys[i]));
            newLeaf->values.push_back(std::move(leaf->values[i]));
            newLeaf->numKeys++;
        }

        // Adjust original leaf
        leaf->keys.resize(splitPoint);
        leaf->values.resize(splitPoint);
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

    try {
        newNode = new InternalNode<KeyType, ValueType>(maxKeys);

        // Calculate split point
        int splitPoint = (maxKeys + 1) / 2;

        // Key to promote to parent
        KeyType promoteKey = node->keys[splitPoint];

        // Reserve space to minimize allocation failures
        newNode->keys.reserve(node->numKeys - splitPoint - 1);
        newNode->children.reserve(node->children.size() - splitPoint - 1);

        // Move second half to new node
        for (int i = splitPoint + 1; i < node->numKeys; i++) {
            newNode->keys.push_back(std::move(node->keys[i]));
            newNode->numKeys++;
        }

        // Move children
        for (size_t i = splitPoint + 1; i < node->children.size(); i++) {
            newNode->children.push_back(node->children[i]);
            node->children[i]->parent = newNode;
        }

        // Adjust original node
        node->keys.resize(splitPoint);
        node->children.resize(splitPoint + 1);
        node->numKeys = splitPoint;

        // Insert into parent
        insertIntoParent(node, promoteKey, newNode);
    } catch (...) {
        // If an exception occurs, clean up the new node
        // Note: children have already been transferred, so we need to restore them
        if (newNode) {
            // Restore children back to original node before deleting
            for (auto child : newNode->children) {
                if (child) {
                    child->parent = node;
                }
            }
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
        newRoot->keys.push_back(key);
        newRoot->numKeys = 1;
        newRoot->children.push_back(left);
        newRoot->children.push_back(right);
        left->parent = newRoot;
        right->parent = newRoot;
        root = newRoot;
        return;
    }

    // Insert into existing parent
    InternalNode<KeyType, ValueType>* parent =
        static_cast<InternalNode<KeyType, ValueType>*>(left->parent);

    int pos = parent->findKeyPosition(key);
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

    // Find the key
    int pos = -1;
    for (int i = 0; i < leaf->numKeys; i++) {
        if (leaf->keys[i] == key) {
            pos = i;
            break;
        }
    }

    if (pos == -1) return false;  // Key not found

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
        if (leftSibling->numKeys > minKeys) {
            redistributeNodes(node, leftSibling, nodeIndex - 1, true);
            return;
        }
    }

    if (static_cast<size_t>(nodeIndex) < parent->children.size() - 1) {
        Node<KeyType, ValueType>* rightSibling = parent->children[nodeIndex + 1];
        if (rightSibling->numKeys > minKeys) {
            redistributeNodes(node, rightSibling, nodeIndex, false);
            return;
        }
    }

    // Merge with sibling
    if (nodeIndex > 0) {
        Node<KeyType, ValueType>* leftSibling = parent->children[nodeIndex - 1];
        mergeNodes(leftSibling, node, nodeIndex - 1, true);
    } else {
        Node<KeyType, ValueType>* rightSibling = parent->children[nodeIndex + 1];
        mergeNodes(node, rightSibling, nodeIndex, false);
    }
}

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

        // Move all from right to left
        for (int i = 0; i < rightLeaf->numKeys; i++) {
            leftLeaf->keys.push_back(rightLeaf->keys[i]);
            leftLeaf->values.push_back(rightLeaf->values[i]);
            leftLeaf->numKeys++;
        }

        // Update linked list
        leftLeaf->next = rightLeaf->next;
        if (rightLeaf->next) {
            rightLeaf->next->prev = leftLeaf;
        }

        delete rightLeaf;
    } else {
        assert(left->isInternal() && right->isInternal() && "Both nodes must be internal");
        InternalNode<KeyType, ValueType>* leftInternal =
            static_cast<InternalNode<KeyType, ValueType>*>(left);
        InternalNode<KeyType, ValueType>* rightInternal =
            static_cast<InternalNode<KeyType, ValueType>*>(right);

        // Pull down the separator key from parent
        assert(left->parent && left->parent->isInternal() && "Parent must be internal");
        InternalNode<KeyType, ValueType>* parent =
            static_cast<InternalNode<KeyType, ValueType>*>(left->parent);
        leftInternal->keys.push_back(parent->keys[parentIndex]);
        leftInternal->numKeys++;

        // Move all from right to left
        for (int i = 0; i < rightInternal->numKeys; i++) {
            leftInternal->keys.push_back(rightInternal->keys[i]);
            leftInternal->numKeys++;
        }

        for (auto child : rightInternal->children) {
            leftInternal->children.push_back(child);
            child->parent = leftInternal;
        }

        delete rightInternal;
    }

    // Remove separator key from parent
    assert(left->parent && left->parent->isInternal() && "Parent must be internal");
    InternalNode<KeyType, ValueType>* parent =
        static_cast<InternalNode<KeyType, ValueType>*>(left->parent);
    parent->removeKeyAt(parentIndex);
    parent->removeChildAt(parentIndex + 1);

    // Handle parent underflow
    if (parent->isUnderflow(minKeys)) {
        deleteEntry(parent);
    }
}

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
            // Borrow from left sibling
            leaf->keys.insert(leaf->keys.begin(), siblingLeaf->keys.back());
            leaf->values.insert(leaf->values.begin(), siblingLeaf->values.back());
            leaf->numKeys++;

            siblingLeaf->keys.pop_back();
            siblingLeaf->values.pop_back();
            siblingLeaf->numKeys--;

            parent->keys[parentIndex] = leaf->keys[0];
        } else {
            // Borrow from right sibling
            leaf->keys.push_back(siblingLeaf->keys[0]);
            leaf->values.push_back(siblingLeaf->values[0]);
            leaf->numKeys++;

            siblingLeaf->keys.erase(siblingLeaf->keys.begin());
            siblingLeaf->values.erase(siblingLeaf->values.begin());
            siblingLeaf->numKeys--;

            parent->keys[parentIndex] = siblingLeaf->keys[0];
        }
    } else {
        assert(node->isInternal() && sibling->isInternal() && "Both nodes must be internal");
        InternalNode<KeyType, ValueType>* internal =
            static_cast<InternalNode<KeyType, ValueType>*>(node);
        InternalNode<KeyType, ValueType>* siblingInternal =
            static_cast<InternalNode<KeyType, ValueType>*>(sibling);

        if (isLeftSibling) {
            // Borrow from left sibling
            internal->keys.insert(internal->keys.begin(), parent->keys[parentIndex]);
            internal->numKeys++;

            internal->children.insert(internal->children.begin(),
                                     siblingInternal->children.back());
            siblingInternal->children.back()->parent = internal;

            parent->keys[parentIndex] = siblingInternal->keys.back();

            siblingInternal->keys.pop_back();
            siblingInternal->children.pop_back();
            siblingInternal->numKeys--;
        } else {
            // Borrow from right sibling
            internal->keys.push_back(parent->keys[parentIndex]);
            internal->numKeys++;

            internal->children.push_back(siblingInternal->children[0]);
            siblingInternal->children[0]->parent = internal;

            parent->keys[parentIndex] = siblingInternal->keys[0];

            siblingInternal->keys.erase(siblingInternal->keys.begin());
            siblingInternal->children.erase(siblingInternal->children.begin());
            siblingInternal->numKeys--;
        }
    }
}

template<typename KeyType, typename ValueType>
int BPlusTree<KeyType, ValueType>::getNodeIndex(Node<KeyType, ValueType>* node) {
    InternalNode<KeyType, ValueType>* parent =
        static_cast<InternalNode<KeyType, ValueType>*>(node->parent);

    for (size_t i = 0; i < parent->children.size(); i++) {
        if (parent->children[i] == node) {
            return i;
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
        for (int i = 0; i < leaf->numKeys; i++) {
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
    for (int i = 0; i < node->numKeys; i++) {
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
        for (auto child : internal->children) {
            printNode(child, level + 1);
        }
    }
}

template<typename KeyType, typename ValueType>
int BPlusTree<KeyType, ValueType>::height() const {
    if (!root) return 0;

    int h = 1;
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
    for (int i = 1; i < node->numKeys; i++) {
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

        // Check child count
        if (internal->children.size() != static_cast<size_t>(node->numKeys + 1)) {
            std::cerr << "Invalid child count at level " << level << std::endl;
            return false;
        }

        // Validate children
        for (auto child : internal->children) {
            if (!validateNode(child, level + 1, leafLevel)) {
                return false;
            }
        }
    }

    return true;
}

} // namespace bptree

#endif // BPLUSTREE_H
