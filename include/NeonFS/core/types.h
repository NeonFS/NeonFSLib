#pragma once
#include <chrono>
#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <array>
#include "secure_allocator.hpp"

namespace neonfs {
    struct Error {
        std::string message;
        int code = 0;
    };

    using secure_string = std::basic_string<char, std::char_traits<char>, secure_allocator<char>>;
    using secure_wstring = std::basic_string<wchar_t, std::char_traits<wchar_t>, secure_allocator<wchar_t>>;

    template<typename T>
    using secure_vector = std::vector<T, secure_allocator<T>>;

    using secure_bytes = secure_vector<uint8_t>;

    template<typename T>
    using secure_list = std::list<T, secure_allocator<T>>;

    template<typename T>
    using secure_deque = std::deque<T, secure_allocator<T>>;

    template<typename Key, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>>
    using secure_unordered_set = std::unordered_set<Key, Hash, KeyEqual, secure_allocator<Key>>;

    template<typename Key, typename T, typename Compare = std::less<Key>>
    using secure_map = std::map<Key, T, Compare, secure_allocator<std::pair<const Key, T>>>;

    template<typename Key, typename T, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>>
    using secure_unordered_map = std::unordered_map<Key, T, Hash, KeyEqual, secure_allocator<std::pair<const Key, T>>>;

    enum class StorageErrorCode : int {
        SUCCESS = 0,
        INVALID_BLOCK_ID,           // Block ID out of range
        BLOCK_NOT_FOUND,            // Requested block doesn't exist
        STORAGE_FULL,               // No space available
        IO_ERROR,                   // Low-level I/O error
        PERMISSION_DENIED,          // Insufficient permissions
        NOT_INITIALIZED,            // Provider not initialized
        ALREADY_EXISTS,             // Block already exists
        TRANSACTION_ERROR,          // Transaction-related error
        INVALID_PARAMETER,          // Invalid input parameters
        UNSUPPORTED_OPERATION,      // Operation not supported by this provider
        TIMEOUT,                    // Operation timed out
        CORRUPTION_DETECTED         // Data corruption detected
    };

    // Storage capacity and usage information
    struct StorageStats {
        uint64_t total_capacity;        // Total storage capacity in bytes
        uint64_t used_space;            // Currently used space in bytes
        uint64_t available_space;       // Available space in bytes
        uint64_t block_count;           // Total number of blocks
        uint64_t block_size;            // Size of each block in bytes
        uint64_t reserved_blocks;       // Number of reserved blocks
        std::chrono::system_clock::time_point last_updated;
    };

    // Storage configuration
    struct StorageConfig {
        std::string resource_uri;
        uint64_t block_size;        // Size of each block in bytes (e.g., 4096)
        uint64_t total_size;        // Total size of the storage file in bytes
        uint64_t initial_capacity;  // Initial capacity (0 = no limit)
        size_t max_concurrent_ops;  // Maximum concurrent operations
    };

} // namespace neonfs