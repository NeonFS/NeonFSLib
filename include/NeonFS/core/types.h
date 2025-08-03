#pragma once
#include <chrono>
#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
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

    struct BlockStorageConfig {
        size_t block_size;
        size_t total_size;
    };

    /**
     * @brief Represents a block entry associated with a file.
     */
    struct BlockInfo {
        uint64_t blockId;                   // Block ID
        uint64_t offset;                    // Offset in file
        std::vector<uint8_t> iv;            // Initialization vector for encryption
        std::vector<uint8_t> tag;           // Authentication tag (GCM)
    };

    /**
     * @brief Represents metadata associated with a file or directory in NeonFS.
     */
    struct Metadata {
        uint64_t fileId;                    // Unique file or directory identifier
        std::string filename;               // Name of the file or directory
        uint64_t size;                      // Total size (0 for directories)
        uint64_t timestamp_created;         // Creation timestamp (epoch)
        uint64_t timestamp_modified;        // Last-modified timestamp (epoch)
        uint32_t permissions;               // Permission bitmask
        bool isDirectory;                   // True if this is a directory
        uint64_t parentId;                  // ID of the parent directory (0 for root)

        std::vector<BlockInfo> blocks;      // Ordered list of associated blocks (empty for directories)
    };

} // namespace neonfs