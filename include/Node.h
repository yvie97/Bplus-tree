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
        // Pre-allocate to maxKeys + 1 to handle overflow during splits
        keys.resize(maxK + 1);
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

    // Insert a key at the specified position using manual shifting
    void insertKeyAt(int pos, const KeyType& key) {
        // Shift elements to the right manually - O(n) but with better cache performance
        for (int i = numKeys; i > pos; --i) {
            keys[i] = std::move(keys[i - 1]);
        }
        keys[pos] = key;
        numKeys++;
    }

    // Remove key at specified position using manual shifting
    void removeKeyAt(int pos) {
        // Shift elements to the left manually - O(n) but with better cache performance
        for (int i = pos; i < numKeys - 1; ++i) {
            keys[i] = std::move(keys[i + 1]);
        }
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
        // Pre-allocate to maxKeys + 3 to handle overflow during splits
        // +3 because: during insertIntoParent, we first increment numKeys (making it maxKeys+1),
        // then call insertChildAt which needs numKeys+1 children (maxKeys+2),
        // and needs to shift one more position (maxKeys+3)
        // Initialize all pointers to nullptr
        children.resize(maxKeys + 3, nullptr);
    }

    ~InternalNode() override {
        // Don't delete children here - tree destructor handles it
    }

    // Insert a child at the specified position using manual shifting
    void insertChildAt(int pos, Node<KeyType, ValueType>* child) {
        // Determine the current number of children
        int numChildren = this->numKeys + 1;

        // Shift children to the right manually
        for (int i = numChildren; i > pos; --i) {
            children[i] = children[i - 1];
        }
        children[pos] = child;
        if (child) {
            child->parent = this;
        }
    }

    // Remove child at specified position using manual shifting
    void removeChildAt(int pos) {
        // Determine the current number of children
        int numChildren = this->numKeys + 1;

        // Shift children to the left manually
        for (int i = pos; i < numChildren - 1; ++i) {
            children[i] = children[i + 1];
        }
        children[numChildren - 1] = nullptr;
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
        // Pre-allocate to maxKeys + 1 to handle overflow during splits
        values.resize(maxKeys + 1);
    }

    ~LeafNode() override = default;

    // Insert key-value pair at specified position using manual shifting
    void insertAt(int pos, const KeyType& key, const ValueType& value) {
        // Shift keys manually
        for (int i = this->numKeys; i > pos; --i) {
            this->keys[i] = std::move(this->keys[i - 1]);
        }
        this->keys[pos] = key;

        // Shift values manually
        for (int i = this->numKeys; i > pos; --i) {
            values[i] = std::move(values[i - 1]);
        }
        values[pos] = value;

        this->numKeys++;
    }

    // Move-based insert for better performance and exception safety
    void insertAt(int pos, KeyType&& key, ValueType&& value) {
        // Shift keys manually
        for (int i = this->numKeys; i > pos; --i) {
            this->keys[i] = std::move(this->keys[i - 1]);
        }
        this->keys[pos] = std::move(key);

        // Shift values manually
        for (int i = this->numKeys; i > pos; --i) {
            values[i] = std::move(values[i - 1]);
        }
        values[pos] = std::move(value);

        this->numKeys++;
    }

    // Remove key-value pair at specified position using manual shifting
    void removeAt(int pos) {
        // Shift keys (using parent class method)
        this->removeKeyAt(pos);

        // Shift values manually
        for (int i = pos; i < this->numKeys; ++i) {
            values[i] = std::move(values[i + 1]);
        }
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
