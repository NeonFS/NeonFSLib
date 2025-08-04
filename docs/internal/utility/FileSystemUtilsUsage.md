# Usage of `file_system_utils`

This guide provides practical examples for using the helper functions in the `neonfs::utility` namespace to perform common path and file name operations safely and reliably.

---

## Example 1: Handling User-Provided Filenames

When accepting filenames from users, it's critical to validate and sanitize them to prevent errors and potential security issues.

```cpp
#include <NeonFS/utility/file_system_utils.h>
#include <iostream>

void process_user_filename(const std::string& user_input) {
    using namespace neonfs::utility;

    // 1. Check if the name contains invalid characters.
    if (!isValidFileName(user_input)) {
        std::cerr << "Error: The filename '" << user_input << "' contains invalid characters." << std::endl;
        return;
    }

    // 2. Check for reserved Windows names (important for cross-platform compatibility).
    if (isReservedWindowsName(user_input)) {
        std::cerr << "Error: '" << user_input << "' is a reserved system name." << std::endl;
        return;
    }

    // 3. Sanitize the name to be absolutely sure it's safe.
    //    For example, if the input was "My Report?.docx", it becomes "My Report_.docx".
    std::string safe_filename = sanitizeFileName(user_input);

    std::cout << "Using safe filename: " << safe_filename << std::endl;
    // Now you can use `safe_filename` to create a file.
}
```

---

## Example 2: Manipulating and Constructing Paths

These utilities simplify building and cleaning up file paths.

```cpp
#include <NeonFS/utility/file_system_utils.h>
#include <iostream>
#include <vector>

void build_and_normalize_path() {
    using namespace neonfs::utility;

    // Start with a messy, non-standard path.
    std::string messy_path = "C:\\Users\\Guest/Documents/../MyStuff/./report.txt";
    
    // Normalize it to its simplest, canonical form.
    std::string clean_path = normalizePath(messy_path);
    // On Windows, clean_path becomes: "C:\\Users\\MyStuff\\report.txt"
    std::cout << "Normalized path: " << clean_path << std::endl;

    // You can also build paths from components.
    std::vector<std::string> parts = {"home", "user", "documents"};
    std::string joined_path = joinPath(parts);
    // On Linux/macOS, joined_path becomes: "home/user/documents"
    std::cout << "Joined path: " << joined_path << std::endl;
}
```

---

## Example 3: Extracting Information from a Path

Easily get the directory, extension, or stem of a file path.

```cpp
#include <NeonFS/utility/file_system_utils.h>
#include <iostream>

void extract_path_info() {
    using namespace neonfs::utility;

    std::string full_path = "/var/log/system.log.1";

    // Get the parent directory
    std::string parent = getParentPath(full_path);
    // parent is "/var/log"
    std::cout << "Parent directory: " << parent << std::endl;

    // Get the file extension
    std::string ext = getFileExtension(full_path);
    // ext is ".1"
    std::cout << "Extension: " << ext << std::endl;

    // Get the path without the extension
    std::string stem = removeFileExtension(full_path);
    // stem is "/var/log/system.log"
    std::cout << "Path without extension: " << stem << std::endl;
}
```