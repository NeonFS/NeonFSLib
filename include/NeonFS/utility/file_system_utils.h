#pragma once
#include <string>
#include <vector>
#include <unordered_set>

namespace neonfs::utility {
    static const std::unordered_set<std::string> reserved_windows_names = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };

    // Sanitizes a file name by removing invalid characters like \ / : * ? " < > |
    std::string sanitizeFileName(const std::string& name);

    // Checks if a file name is valid (no reserved characters, not reserved Windows names, etc.)
    bool isValidFileName(const std::string& name);

    // Splits a path into segments using '/' or '\'
    std::vector<std::string> splitPath(const std::string& path);

    // Joins path segments safely
    std::string joinPath(const std::vector<std::string>& parts);

    // Gets the file extension in lowercase includes the dot (e.g. ".txt")
    std::string getFileExtension(const std::string& filename);

    // Removes the extension from the path (returns path without file extension)
    std::string removeFileExtension(const std::string& path);

    // Normalizes a path: removes redundant slashes, dots, and ensures consistent separators
    std::string normalizePath(const std::string& path);

    // Makes a path absolute relative to a base
    std::string makeAbsolutePath(const std::string& base, const std::string& relative);

    // Returns the parent directory of a given path
    std::string getParentPath(const std::string& path);

    // Checks if the given name is one of the reserved Windows device names (e.g. CON, NUL, COM1, etc.)
    bool isReservedWindowsName(const std::string& name);
} // namespace neonfs::utility