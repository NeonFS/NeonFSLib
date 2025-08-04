#include <NeonFS/utility/file_system_utils.h>
#include <regex>
#include <sstream>
#include <filesystem>

std::string neonfs::utility::sanitizeFileName(const std::string &name) {
    static const std::regex invalid(R"([\\/:*?"<>|\x00-\x1F])"); // invalid chars + control characters
    const std::string sanitized = std::regex_replace(name, invalid, "_");

    // Trim spaces at beginning and end
    const size_t start = sanitized.find_first_not_of(' ');
    const size_t end = sanitized.find_last_not_of(' ');
    if (start == std::string::npos) return ""; // All spaces
    return sanitized.substr(start, end - start + 1);
}

bool neonfs::utility::isValidFileName(const std::string &name) {
    return !sanitizeFileName(name).empty(); // If result is empty, it was invalid
}

std::vector<std::string> neonfs::utility::splitPath(const std::string &path) {
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string item;

    // Treat both / and \ as delimiters
    for (const char ch : path) {
        if (ch == '/' || ch == '\\') {
            if (!item.empty()) {
                parts.push_back(item);
                item.clear();
            }
        } else {
            item += ch;
        }
    }
    if (!item.empty()) parts.push_back(item);

    return parts;
}

std::string neonfs::utility::joinPath(const std::vector<std::string> &parts) {
    if (parts.empty()) return "";

    std::ostringstream oss;
    auto it = parts.begin();
    oss << *it; // Add first part without separator
    ++it;

    // Append the rest with separator
    for (; it != parts.end(); ++it) {
        oss << static_cast<char>(std::filesystem::path::preferred_separator) << *it;
    }
    return oss.str();
}

std::string neonfs::utility::getFileExtension(const std::string &filename) {
    return std::filesystem::path(filename).extension().string();
}

std::string neonfs::utility::removeFileExtension(const std::string &path) {
    std::filesystem::path p(path);
    p.replace_extension();
    return p.string();
}

std::string neonfs::utility::normalizePath(const std::string &path) {
    try {
        return std::filesystem::weakly_canonical(std::filesystem::path(path)).string(); // Tolerant resolution
    } catch (...) {
        return path; // Return original if path is invalid
    }
}

std::string neonfs::utility::makeAbsolutePath(const std::string &base, const std::string &relative) {
    const std::filesystem::path base_path(base);
    const std::filesystem::path relative_path(relative);
    return (base_path / relative_path).string();
}

std::string neonfs::utility::getParentPath(const std::string &path) {
    const std::filesystem::path p(path);
    return p.parent_path().string();
}

bool neonfs::utility::isReservedWindowsName(const std::string &name) {
    // Extract base name (remove extension)
    std::string base = name;
    size_t dot_pos = base.find('.');
    if (dot_pos != std::string::npos) {
        base = base.substr(0, dot_pos);
    }

    // Convert to uppercase
    std::ranges::transform(base, base.begin(), [](char c) {
        return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    });

    return reserved_windows_names.contains(base);
}
