# Usage of `secure_allocator<T>`

This document explains how to effectively use the `neonfs::secure_allocator<T>` with practical examples and container integration patterns.

---

## Prerequisite: Heap Initialization

**WARNING:** Before you can use any secure container (`secure_bytes`, `secure_string`, etc.), you **must** initialize the OpenSSL secure heap. This is a one-time operation that should happen at application startup.

Failure to initialize the heap will result in a `std::runtime_error` or `std::bad_alloc` when you attempt to create a secure container.

### How to Initialize
Place the following call at the beginning of your `main()` function:

```cpp
#include <NeonFS/core/types.h> // Provides initialize_secure_heap
#include <iostream>

int main() {
    try {
        // Initialize a 64MB secure heap. This size must be large
        // enough for your application's peak secure memory needs.
        neonfs::initialize_secure_heap(64 * 1024 * 1024);
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    // --- Your application logic can now safely use secure containers ---
    
    return 0;
}
```
---

## Declaring a Secure Container

The primary use case for `secure_allocator<T>` is in STL containers that hold sensitive data.
You can use it with standard containers like `std::vector`, `std::string`, or `std::list`.

### Typedef Example

```cpp
using secure_buffer = std::vector<uint8_t, neonfs::secure_allocator<uint8_t>>;
```

This creates a type alias `secure_buffer` that can hold binary data (like a cryptographic key) in a secure memory region.

---

## Built-in Secure Containers

To simplify working with sensitive data, `NeonFSLib` provides a set of pre-defined, ready-to-use type aliases for the most common STL containers. These are all available in the `<NeonFS/core/types.h>` header.

You should prefer these aliases over creating manual typedefs.

| Type                         | Description                                                                    |
| ---------------------------- | ------------------------------------------------------------------------------ |
| `neonfs::secure_string`      | For sensitive `char` strings like passwords or tokens.                         |
| `neonfs::secure_wstring`     | For sensitive wide-character strings.                                          |
| `neonfs::secure_bytes`       | A vector of `uint8_t`, ideal for binary data like cryptographic keys or buffers. |
| `neonfs::secure_vector<T>`   | A secure vector for any trivially destructible type `T`.                       |
| `neonfs::secure_list<T>`     | A secure list.                                                                 |
| `neonfs::secure_map<K, V>`   | A secure, ordered map.                                                         |
| `neonfs::secure_unordered_map<K, V>` | A secure, unordered map.                                                     |

> **Note:** These containers can only be used with **trivially destructible types**.

---

## Example: Storing a Secret Key

Use `neonfs::secure_bytes` to hold binary secrets. The memory is automatically wiped when the object goes out of scope.

```cpp
#include <NeonFS/core/types.h>

// Create a 32-byte buffer for an AES-256 key.
neonfs::secure_bytes key(32);

// ... fill the key with random data from a secure source ...

// The memory allocated for `key` now lives in the secure heap.

```

---

## Example: Storing a Password

Use `neonfs::secure_string` for handling passwords or other sensitive text.

```cpp
#include <NeonFS/core/types.h>

neonfs::secure_string password;
password.assign("SuperSecretPassword123!");

// When `password` is destroyed, its memory is securely wiped.
```

---

## Example: Reading Sensitive Data into Secure Memory

You can read data directly into a secure container, ensuring it never touches the standard heap.

```cpp
#include <NeonFS/core/types.h>
#include <NeonFS/core/result.hpp>
#include <fstream>

Result<neonfs::secure_bytes> read_secret_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return Result<neonfs::secure_bytes>::err("Unable to open file", errno);
    }

    // Read file contents directly into a secure buffer
    neonfs::secure_bytes contents((std::istreambuf_iterator<char>(file)),
                                  (std::istreambuf_iterator<char>()));

    return Result<neonfs::secure_bytes>::ok(std::move(contents));
}
```

---

## Manually Clearing Secure Containers

The `secure_allocator` automatically wipes memory upon deallocation. However, it is a good practice to wipe sensitive data **immediately** after it is no longer needed. This provides defense-in-depth by minimizing the time the secret exists in memory.

```cpp
#include <NeonFS/core/types.h>
#include <algorithm>

void process_sensitive_token() {
    neonfs::secure_bytes token(64);
    // ... use the token ...

    // Wipe the token immediately after use.
    std::fill(token.begin(), token.end(), 0);

    // The container can now be reused or will be securely deallocated later.
}
```

---

## STL Container Compatibility

Your allocator is designed to work seamlessly with most STL containers. Common secure container types:

```cpp
using secure_string = std::basic_string<char, std::char_traits<char>, neonfs::secure_allocator<char>>;
using secure_vector = std::vector<uint8_t, neonfs::secure_allocator<uint8_t>>;
using secure_int_list = std::list<int, neonfs::secure_allocator<int>>;
```

> **Note:** Only use it with trivially destructible types like `char`, `uint8_t`, `int`, etc.

---

## Best Practices

*   **Initialize the secure heap** in `main()` before any other operations.
*   **Use type aliases** like `secure_bytes` or `secure_string` to improve code readability and safety.
*   **Use for any in-memory data that is security-critical**: keys, passwords, tokens, etc.
*   **Do not use** with non-trivially-destructible types (e.g., classes with custom destructors, `std::unique_ptr`).
*   **Wipe sensitive data immediately** after use as a defense-in-depth measure.

---

## Security Notes

* Memory is allocated via `OPENSSL_secure_malloc()`, which:
    * Prevents paging to disk (locked pages)
    * Guards memory from being reused insecurely
* On deallocation, the memory is securely wiped via `OPENSSL_secure_clear_free()`.

---

## Limitations

* Only supports **trivially destructible types**.
* STL containers must use allocator-aware constructors or typedefs.
* `secure_allocator` should not be used with types requiring complex destruction or copy semantics.

---

For allocator design details, see [SecureAllocator.md](SecureAllocator.md).