# `file_system_utils` — Filesystem Utilities

---
namespace:
- `neonfs::utility`
---

## Overview

The `neonfs::utility` namespace provides a collection of freestanding, stateless functions for common filesystem path and name manipulations. These helpers are designed to offer a robust, cross-platform way to handle file and directory paths, ensuring that generated names and paths are valid and normalized.

The implementation leverages the C++17 `std::filesystem` library where appropriate but provides a simplified, string-based interface for common operations.

---

## API Reference

All functions are available directly under the `neonfs::utility` namespace.

### Validation and Sanitization

**`std::string sanitizeFileName(const std::string& name)`**
Removes characters that are invalid in file names on most operating systems (e.g., `\`, `/`, `:`, `*`, `?`, `"`, `<`, `>`, `|`) and replaces them with an underscore (`_`). It also trims leading/trailing whitespace.

**`bool isValidFileName(const std::string& name)`**
Checks if a file name is valid. A name is considered invalid if it is empty after sanitization, meaning it consisted only of invalid characters or whitespace.

**`bool isReservedWindowsName(const std::string& name)`**
Checks if a name matches one of the reserved device names on Windows (e.g., `CON`, `NUL`, `COM1`, `LPT1`), ignoring case and file extensions.

### Path Composition and Decomposition

**`std::vector<std::string> splitPath(const std::string& path)`**
Splits a path string into its constituent parts, using both `/` and `\` as delimiters.

**`std::string joinPath(const std::vector<std::string>& parts)`**
Joins a vector of path segments into a single path string, using the platform's preferred separator (`/` on POSIX, `\` on Windows).

**`std::string getParentPath(const std::string& path)`**
Returns the parent directory portion of a given path.

### Path Transformation

**`std::string normalizePath(const std::string& path)`**
Normalizes a path by resolving `.` and `..` components, removing redundant separators, and standardizing separator characters. It performs a "weak" canonicalization, meaning the path does not need to exist on disk.

**`std::string makeAbsolutePath(const std::string& base, const std::string& relative)`**
Constructs an absolute path by appending a relative path to a base path.

### File Name Operations

**`std::string getFileExtension(const std::string& filename)`**
Returns the file extension from a filename, including the leading dot (e.g., `.txt`). If there is no extension, it returns an empty string.

**`std::string removeFileExtension(const std::string& path)`**
Returns the path or filename with its extension removed.
