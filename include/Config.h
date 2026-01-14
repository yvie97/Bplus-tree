#ifndef BPLUSTREE_CONFIG_H
#define BPLUSTREE_CONFIG_H

/**
 * @file Config.h
 * @brief Configuration constants for the B+ tree implementation
 */

namespace bptree {

/**
 * @brief Default order for B+ tree (can be overridden at runtime)
 *
 * The order of a B+ tree determines the maximum number of children per node.
 * A node of order m can have at most m children and m-1 keys.
 * This default value balances memory usage with tree height.
 *
 * With order 4:
 * - Internal nodes: up to 3 keys and 4 children
 * - Leaf nodes: up to 3 key-value pairs
 * - Minimum keys per node (except root): ceil(4/2) - 1 = 1
 */
constexpr int DEFAULT_ORDER = 4;

/**
 * @brief Minimum order allowed for B+ tree
 *
 * Orders less than 3 would not provide the benefits of a B+ tree structure.
 * Order 3 is the smallest valid order, giving:
 * - Internal nodes: up to 2 keys and 3 children
 * - Leaf nodes: up to 2 key-value pairs
 * - Minimum keys per node (except root): ceil(3/2) - 1 = 1
 */
constexpr int MIN_ORDER = 3;

} // namespace bptree

#endif // BPLUSTREE_CONFIG_H
