#ifndef BPLUSTREE_NODE_H
#define BPLUSTREE_NODE_H

#include <vector>
#include <memory>

namespace bptree {

/**
 * @brief Enumeration for node types in the B+ tree
 */
enum class NodeType {
    INTERNAL,  ///< Internal (non-leaf) node that contains only keys and child pointers
    LEAF       ///< Leaf node that contains key-value pairs and is linked to adjacent leaves
};

// Forward declarations
template<typename KeyType, typename ValueType>
class InternalNode;

template<typename KeyType, typename ValueType>
class LeafNode;

/**
 * @brief Base class for all nodes in the B+ tree
 *
 * Provides common functionality for both internal and leaf nodes, including
 * key management, capacity checking, and position finding.
 *
 * @tparam KeyType The type of keys stored in the node
 * @tparam ValueType The type of values (used by leaf nodes)
 */
template<typename KeyType, typename ValueType>
class Node {
public:
    NodeType type;           ///< Type of this node (INTERNAL or LEAF)
    int numKeys;             ///< Current number of keys in this node
    std::vector<KeyType> keys;  ///< Array of keys (sorted)
    Node* parent;            ///< Pointer to parent node (nullptr for root)
    int maxKeys;             ///< Maximum number of keys this node can hold

    /**
     * @brief Constructs a node with the specified type and maximum capacity
     *
     * @param t The type of node (INTERNAL or LEAF)
     * @param maxK Maximum number of keys (order - 1)
     */
    Node(NodeType t, int maxK)
        : type(t), numKeys(0), parent(nullptr), maxKeys(maxK) {
        // Pre-allocate to maxKeys + 1 to handle overflow during splits
        keys.resize(maxK + 1);
    }

    /**
     * @brief Virtual destructor for proper cleanup of derived classes
     */
    virtual ~Node() = default;

    /**
     * @brief Checks if this is a leaf node
     * @return true if this is a leaf node, false otherwise
     */
    bool isLeaf() const {
        return type == NodeType::LEAF;
    }

    /**
     * @brief Checks if this is an internal node
     * @return true if this is an internal node, false otherwise
     */
    bool isInternal() const {
        return type == NodeType::INTERNAL;
    }

    /**
     * @brief Checks if the node has exceeded its capacity
     *
     * A node is full when it has more than maxKeys keys, which happens
     * temporarily during insertion before a split occurs.
     *
     * @return true if numKeys > maxKeys, false otherwise
     */
    bool isFull() const {
        return numKeys > maxKeys;
    }

    /**
     * @brief Checks if the node has fewer than the minimum required keys
     *
     * Used during deletion to determine if rebalancing is needed.
     *
     * @param minKeys The minimum number of keys required
     * @return true if numKeys < minKeys, false otherwise
     */
    bool isUnderflow(int minKeys) const {
        return numKeys < minKeys;
    }

    /**
     * @brief Finds the position where a key exists or should be inserted
     *
     * Uses binary search to find the correct position in O(log numKeys) time.
     * If the key exists, returns its index. If not, returns the index where
     * it should be inserted to maintain sorted order.
     *
     * @param key The key to search for
     * @return The index where the key is or should be inserted
     *
     * Time complexity: O(log numKeys)
     */
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

    /**
     * @brief Inserts a key at the specified position
     *
     * Shifts all keys from the position to the right to make room for the new key.
     * Uses manual array manipulation for better cache performance.
     *
     * @param pos The position to insert at (must be in range [0, numKeys])
     * @param key The key to insert
     *
     * Time complexity: O(numKeys)
     */
    void insertKeyAt(int pos, const KeyType& key) {
        // Shift elements to the right manually - O(n) but with better cache performance
        for (int i = numKeys; i > pos; --i) {
            keys[i] = std::move(keys[i - 1]);
        }
        keys[pos] = key;
        numKeys++;
    }

    /**
     * @brief Removes a key at the specified position
     *
     * Shifts all keys after the position to the left to fill the gap.
     * Uses manual array manipulation for better cache performance.
     *
     * @param pos The position to remove from (must be in range [0, numKeys))
     *
     * Time complexity: O(numKeys)
     */
    void removeKeyAt(int pos) {
        // Shift elements to the left manually - O(n) but with better cache performance
        for (int i = pos; i < numKeys - 1; ++i) {
            keys[i] = std::move(keys[i + 1]);
        }
        numKeys--;
    }
};

/**
 * @brief Internal node class for the B+ tree
 *
 * Internal nodes contain only keys and child pointers (no values).
 * They guide searches down to the leaf level. In a B+ tree internal node,
 * keys[i] represents the smallest key in the subtree rooted at children[i+1].
 *
 * Invariant: An internal node with n keys has n+1 children.
 *
 * @tparam KeyType The type of keys stored in the node
 * @tparam ValueType The type of values (not stored in internal nodes, only used for type consistency)
 */
template<typename KeyType, typename ValueType>
class InternalNode : public Node<KeyType, ValueType> {
public:
    std::vector<Node<KeyType, ValueType>*> children;  ///< Array of child node pointers

    /**
     * @brief Constructs an internal node
     *
     * Pre-allocates space for child pointers to handle temporary overflow during splits.
     *
     * @param maxKeys Maximum number of keys (order - 1)
     */
    InternalNode(int maxKeys)
        : Node<KeyType, ValueType>(NodeType::INTERNAL, maxKeys) {
        // Pre-allocate to maxKeys + 3 to handle overflow during splits
        // +3 because: during insertIntoParent, we first increment numKeys (making it maxKeys+1),
        // then call insertChildAt which needs numKeys+1 children (maxKeys+2),
        // and needs to shift one more position (maxKeys+3)
        // Initialize all pointers to nullptr
        children.resize(maxKeys + 3, nullptr);
    }

    /**
     * @brief Destructor for internal node
     *
     * Does not delete child nodes - the tree's destroyTree() method handles that.
     */
    ~InternalNode() override {
        // Don't delete children here - tree destructor handles it
    }

    /**
     * @brief Inserts a child pointer at the specified position
     *
     * Shifts all child pointers from the position to the right to make room.
     * Updates the child's parent pointer to point to this node.
     *
     * @param pos The position to insert at (must be in range [0, numKeys+1])
     * @param child The child node pointer to insert
     *
     * Time complexity: O(numKeys)
     */
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

    /**
     * @brief Removes a child pointer at the specified position
     *
     * Shifts all child pointers after the position to the left to fill the gap.
     *
     * @param pos The position to remove from (must be in range [0, numKeys])
     *
     * Time complexity: O(numKeys)
     */
    void removeChildAt(int pos) {
        // Determine the current number of children
        int numChildren = this->numKeys + 1;

        // Shift children to the left manually
        for (int i = pos; i < numChildren - 1; ++i) {
            children[i] = children[i + 1];
        }
        children[numChildren - 1] = nullptr;
    }

    /**
     * @brief Finds which child pointer to follow for a given key
     *
     * In B+ tree internal nodes, keys[i] is the smallest key in children[i+1].
     * This function finds the first key greater than the search key, and returns
     * the index of the child to the left of that key.
     *
     * Example: For keys [10, 20, 30] and search key 15:
     *          - Key 10 <= 15, continue
     *          - Key 20 > 15, return index 1 (child between 10 and 20)
     *
     * @param key The key to search for
     * @return The index of the child pointer to follow (in range [0, numKeys])
     *
     * Time complexity: O(numKeys)
     */
    int findChildIndex(const KeyType& key) const {
        int i = 0;
        while (i < this->numKeys && key >= this->keys[i]) {
            i++;
        }
        return i;
    }
};

/**
 * @brief Leaf node class for the B+ tree
 *
 * Leaf nodes contain key-value pairs and form a doubly-linked list for
 * efficient sequential access and range queries. All actual data in the
 * B+ tree is stored in leaf nodes.
 *
 * @tparam KeyType The type of keys stored in the node
 * @tparam ValueType The type of values associated with keys
 */
template<typename KeyType, typename ValueType>
class LeafNode : public Node<KeyType, ValueType> {
public:
    std::vector<ValueType> values;  ///< Array of values corresponding to keys
    LeafNode* next;                 ///< Pointer to next leaf in linked list (for range queries)
    LeafNode* prev;                 ///< Pointer to previous leaf in linked list (for reverse traversal)

    /**
     * @brief Constructs a leaf node
     *
     * Initializes the linked list pointers and pre-allocates space for values.
     *
     * @param maxKeys Maximum number of keys (order - 1)
     */
    LeafNode(int maxKeys)
        : Node<KeyType, ValueType>(NodeType::LEAF, maxKeys),
          next(nullptr), prev(nullptr) {
        // Pre-allocate to maxKeys + 1 to handle overflow during splits
        values.resize(maxKeys + 1);
    }

    /**
     * @brief Destructor for leaf node
     */
    ~LeafNode() override = default;

    /**
     * @brief Inserts a key-value pair at the specified position (copy version)
     *
     * Shifts all keys and values from the position to the right to make room.
     *
     * @param pos The position to insert at (must be in range [0, numKeys])
     * @param key The key to insert
     * @param value The value to insert
     *
     * Time complexity: O(numKeys)
     */
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

    /**
     * @brief Inserts a key-value pair at the specified position (move version)
     *
     * Move version for better performance and exception safety when working
     * with movable types. Shifts all keys and values from the position to the right.
     *
     * @param pos The position to insert at (must be in range [0, numKeys])
     * @param key The key to insert (moved from)
     * @param value The value to insert (moved from)
     *
     * Time complexity: O(numKeys)
     */
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

    /**
     * @brief Removes a key-value pair at the specified position
     *
     * Shifts all keys and values after the position to the left to fill the gap.
     *
     * @param pos The position to remove from (must be in range [0, numKeys))
     *
     * Time complexity: O(numKeys)
     */
    void removeAt(int pos) {
        // Shift keys (using parent class method)
        this->removeKeyAt(pos);

        // Shift values manually
        for (int i = pos; i < this->numKeys; ++i) {
            values[i] = std::move(values[i + 1]);
        }
    }

    /**
     * @brief Searches for a key and retrieves its associated value
     *
     * Linear search through the keys array. Returns true and sets the output
     * parameter if found, otherwise returns false.
     *
     * @param key The key to search for
     * @param value Output parameter that will be set to the value if found
     * @return true if the key was found, false otherwise
     *
     * Time complexity: O(numKeys)
     */
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
