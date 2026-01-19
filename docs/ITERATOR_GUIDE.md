# B+ Tree Iterator Guide

## Overview

The B+ Tree now supports STL-style iterators, allowing you to traverse the tree in sorted order using familiar C++ idioms. The iterator implementation provides bidirectional traversal with full compatibility with STL algorithms.

## Features

- **Bidirectional Iterators**: Move forward and backward through the tree
- **Const Iterators**: Iterate over const trees without modification
- **Reverse Iterators**: Traverse the tree in descending order
- **STL Compatibility**: Works with standard algorithms like `std::find_if`, `std::count_if`, etc.
- **Range-Based For Loops**: Supports modern C++ syntax

## Iterator Types

### Forward Iterators
- `iterator` - Mutable iterator for non-const trees
- `const_iterator` - Const iterator for const trees

### Reverse Iterators
- `reverse_iterator` - Reverse iteration in descending order
- `const_reverse_iterator` - Const reverse iteration

## Basic Usage

### Forward Iteration

```cpp
BPlusTree<int, std::string> tree(4);
tree.insert(10, "ten");
tree.insert(20, "twenty");
tree.insert(30, "thirty");

// Range-based for loop (recommended)
for (const auto& pair : tree) {
    std::cout << pair.first << " => " << pair.second << "\n";
}

// Explicit iterators
for (auto it = tree.begin(); it != tree.end(); ++it) {
    std::cout << it->first << " => " << it->second << "\n";
}
```

### Reverse Iteration

```cpp
// Iterate in descending order
for (auto it = tree.rbegin(); it != tree.rend(); ++it) {
    std::cout << it->first << " => " << it->second << "\n";
}
```

### Const Iteration

```cpp
const BPlusTree<int, std::string>& const_tree = tree;

// Using cbegin/cend
for (auto it = const_tree.cbegin(); it != const_tree.cend(); ++it) {
    std::cout << it->first << " => " << it->second << "\n";
}

// Using crbegin/crend for const reverse iteration
for (auto it = const_tree.crbegin(); it != const_tree.crend(); ++it) {
    std::cout << it->first << " => " << it->second << "\n";
}
```

### Bidirectional Movement

```cpp
auto it = tree.begin();

// Move forward
++it;  // Pre-increment
it++;  // Post-increment

// Move backward
--it;  // Pre-decrement
it--;  // Post-decrement
```

## Using with STL Algorithms

The iterators are fully compatible with STL algorithms:

```cpp
// Count elements
auto count = std::distance(tree.begin(), tree.end());

// Find an element
auto it = std::find_if(tree.begin(), tree.end(),
                       [](const auto& pair) { return pair.first == 50; });

// Count elements matching a condition
auto count_large = std::count_if(tree.begin(), tree.end(),
                                  [](const auto& pair) { return pair.first > 100; });

// Check if all elements satisfy a condition
bool all_positive = std::all_of(tree.begin(), tree.end(),
                                 [](const auto& pair) { return pair.first > 0; });

// Apply a function to all elements
std::for_each(tree.begin(), tree.end(),
              [](const auto& pair) {
                  std::cout << pair.first << ": " << pair.second << "\n";
              });
```

## Available Methods

| Method | Description | Time Complexity |
|--------|-------------|-----------------|
| `begin()` | Returns iterator to first element | O(log n) |
| `end()` | Returns iterator past last element | O(log n) |
| `cbegin()` | Returns const iterator to first element | O(log n) |
| `cend()` | Returns const iterator past last element | O(log n) |
| `rbegin()` | Returns reverse iterator to last element | O(log n) |
| `rend()` | Returns reverse iterator before first element | O(1) |
| `crbegin()` | Returns const reverse iterator to last element | O(log n) |
| `crend()` | Returns const reverse iterator before first element | O(1) |

## Iterator Operations

| Operation | Description | Time Complexity |
|-----------|-------------|-----------------|
| `++it` | Move to next element (forward) | O(1) amortized |
| `--it` | Move to previous element (backward) | O(1) amortized |
| `*it` | Dereference to get key-value pair | O(1) |
| `it->first` | Access key | O(1) |
| `it->second` | Access value | O(1) |
| `it1 == it2` | Check equality | O(1) |
| `it1 != it2` | Check inequality | O(1) |

## Implementation Details

### Iterator Category
The iterators implement the **bidirectional iterator** concept, supporting both forward and backward traversal but not random access.

### Value Type
Iterators return `std::pair<KeyType, ValueType>` where:
- `first` contains the key
- `second` contains the value

### Traversal Mechanism
The iterators leverage the B+ tree's doubly-linked leaf nodes for efficient sequential access:
- Forward iteration follows the `next` pointers between leaf nodes
- Backward iteration follows the `prev` pointers between leaf nodes
- This ensures O(1) amortized time for increment/decrement operations

### Memory and Performance
- Iterators are lightweight, storing only a leaf node pointer and an index
- A cached pair is maintained for dereference operations
- No dynamic memory allocation during iteration
- Iterator operations are exception-safe

## Examples

See `examples/iterator_demo.cpp` for a comprehensive demonstration of iterator usage.

## Testing

Comprehensive tests are available in `tests/test_iterator.cpp`, covering:
- Empty tree iteration
- Single and multiple element iteration
- Range-based for loops
- Const and reverse iterators
- Bidirectional movement
- STL algorithm compatibility
- Iteration across leaf node boundaries
- Iteration after tree modifications

Run tests with:
```bash
cd build
make test_iterator
./test_iterator
```

## Limitations

1. **Not Random Access**: The iterators do not support `it + n` or `it[n]` operations
2. **Iterator Invalidation**: Iterators may be invalidated by insertions or deletions that cause node splits or merges
3. **No Multi-pass Guarantee**: While the iterators support bidirectional traversal, they should be considered input iterators for safety

## Best Practices

1. **Use Range-Based For Loops**: For simple iteration, prefer range-based for loops for cleaner code
2. **Prefer Const Iteration**: Use `const auto&` in range-based loops to avoid unnecessary copies
3. **Check for End**: Always compare iterators with `end()` before dereferencing
4. **Be Aware of Invalidation**: Avoid modifying the tree while iterating

## Migration Guide

If you were previously using `rangeQuery()` for iteration:

**Before:**
```cpp
auto results = tree.rangeQuery(min_key, max_key);
for (const auto& pair : results) {
    // process pair
}
```

**After (if you need all elements):**
```cpp
for (const auto& pair : tree) {
    // process pair directly, no intermediate vector
}
```

**After (if you need a range):**
```cpp
// Still use rangeQuery for specific ranges
auto results = tree.rangeQuery(min_key, max_key);
```

The iterator approach is more memory-efficient as it doesn't create an intermediate vector.
