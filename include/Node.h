#ifndef BPLUSTREE_NODE_H
#define BPLUSTREE_NODE_H

#include <vector>
#include <memory>

namespace bptree {

enum class NodeType {
    INTERNAL,
    LEAF
};

// Forward declarations
template<typename KeyType, typename ValueType>
class InternalNode;

template<typename KeyType, typename ValueType>
class LeafNode;

// Base Node class
template<typename KeyType, typename ValueType>
class Node {
public:
    NodeType type;
    int numKeys;
    std::vector<KeyType> keys;
    Node* parent;
    int maxKeys;

    Node(NodeType t, int maxK)
        : type(t), numKeys(0), parent(nullptr), maxKeys(maxK) {
        keys.reserve(maxK);
    }

    virtual ~Node() = default;

    bool isLeaf() const {
        return type == NodeType::LEAF;
    }

    bool isInternal() const {
        return type == NodeType::INTERNAL;
    }

    bool isFull() const {
        return numKeys > maxKeys;
    }

    bool isUnderflow(int minKeys) const {
        return numKeys < minKeys;
    }

    // Find the position where a key should be inserted or where it exists
    // Returns the index where key is or should be
    int findKeyPosition(const KeyType& key) const {
        int left = 0, right = numKeys - 1;
        int result = numKeys; // Default: insert at end

        // Binary search for the position
        while (left <= right) {
            int mid = left + (right - left) / 2;
            if (keys[mid] == key) {
                return mid;
            } else if (keys[mid] < key) {
                left = mid + 1;
            } else {
                result = mid;
                right = mid - 1;
            }
        }
        return result;
    }

    // Insert a key at the specified position
    void insertKeyAt(int pos, const KeyType& key) {
        keys.insert(keys.begin() + pos, key);
        numKeys++;
    }

    // Remove key at specified position
    void removeKeyAt(int pos) {
        keys.erase(keys.begin() + pos);
        numKeys--;
    }
};

// Internal Node class
template<typename KeyType, typename ValueType>
class InternalNode : public Node<KeyType, ValueType> {
public:
    std::vector<Node<KeyType, ValueType>*> children;

    InternalNode(int maxKeys)
        : Node<KeyType, ValueType>(NodeType::INTERNAL, maxKeys) {
        children.reserve(maxKeys + 1);
    }

    ~InternalNode() override {
        // Don't delete children here - tree destructor handles it
    }

    // Insert a child at the specified position
    void insertChildAt(int pos, Node<KeyType, ValueType>* child) {
        children.insert(children.begin() + pos, child);
        if (child) {
            child->parent = this;
        }
    }

    // Remove child at specified position
    void removeChildAt(int pos) {
        children.erase(children.begin() + pos);
    }

    // Find which child pointer to follow for a given key
    // In B+ tree internal nodes: keys[i] is the smallest key in children[i+1]
    // So we find the first key that is greater than the search key
    int findChildIndex(const KeyType& key) const {
        int i = 0;
        while (i < this->numKeys && key >= this->keys[i]) {
            i++;
        }
        return i;
    }
};

// Leaf Node class
template<typename KeyType, typename ValueType>
class LeafNode : public Node<KeyType, ValueType> {
public:
    std::vector<ValueType> values;
    LeafNode* next;
    LeafNode* prev;

    LeafNode(int maxKeys)
        : Node<KeyType, ValueType>(NodeType::LEAF, maxKeys),
          next(nullptr), prev(nullptr) {
        values.reserve(maxKeys);
    }

    ~LeafNode() override = default;

    // Insert key-value pair at specified position
    void insertAt(int pos, const KeyType& key, const ValueType& value) {
        // Reserve space first to reduce chance of allocation failure mid-operation
        this->keys.reserve(this->numKeys + 1);
        values.reserve(this->numKeys + 1);
        this->insertKeyAt(pos, key);
        values.insert(values.begin() + pos, value);
    }

    // Move-based insert for better performance and exception safety
    void insertAt(int pos, KeyType&& key, ValueType&& value) {
        this->keys.reserve(this->numKeys + 1);
        values.reserve(this->numKeys + 1);
        this->keys.insert(this->keys.begin() + pos, std::move(key));
        this->numKeys++;
        values.insert(values.begin() + pos, std::move(value));
    }

    // Remove key-value pair at specified position
    void removeAt(int pos) {
        this->removeKeyAt(pos);
        values.erase(values.begin() + pos);
    }

    // Find value for a given key
    bool findValue(const KeyType& key, ValueType& value) const {
        for (int i = 0; i < this->numKeys; i++) {
            if (this->keys[i] == key) {
                value = values[i];
                return true;
            }
        }
        return false;
    }
};

} // namespace bptree

#endif // BPLUSTREE_NODE_H
