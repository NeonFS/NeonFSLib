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

} // namespace neonfs