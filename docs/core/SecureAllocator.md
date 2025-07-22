# `secure_allocator<T>` — Secure Memory Allocator for Sensitive Data

---
namespace:
- `neonfs`
---

## Why `secure_allocator<T>`?

`secure_allocator` is a C++ allocator designed to securely manage memory for sensitive data like cryptographic keys, authentication tokens, and passwords. It leverages OpenSSL’s secure heap to ensure that this memory:

*   **Is never paged to disk**, even under memory pressure.
*   **Is securely wiped** (zeroed) before being deallocated.
*   **Is protected from certain memory-scraping attacks** and forensic recovery attempts.

It provides a standard STL-compatible interface, making it easy to integrate into modern C++ codebases.

---

## What Problems Does It Solve?

* **Memory Scraping Protection**: Ensures sensitive memory is cleared before deallocation to prevent leftover secrets.
* **Page-Locking & Guarding**: OpenSSL's secure heap prevents memory from being swapped to disk, protecting secrets under high memory pressure.
* **Safe Deallocation**: Automatically wipes memory using `OPENSSL_secure_clear_free` before freeing it.
* **Standard Interface**: Compatible with STL containers (when implemented to match the allocator API), making it easy to use in real-world code.

---

## How Does It Work?

`secure_allocator<T>` is a drop-in allocator with STL-style semantics (i.e., via `std::allocator_traits`) that uses OpenSSL's secure heap under the hood.

### Key Operations

* **Allocation:**

  ```cpp
  T* allocate(std::size_t n);
  ```

  Uses `OPENSSL_secure_malloc(n * sizeof(T))`. Throws `std::bad_alloc` on failure.

* **Deallocation:**

  ```cpp
  void deallocate(T* p, std::size_t n) noexcept;
  ```

  Calls `OPENSSL_secure_clear_free(p, n * sizeof(T))` to zero memory and release it.

* **Rebinding:**

  ```cpp
  template<typename U>
  struct rebind { using other = secure_allocator<U>; };
  ```

* **Constraints:**
  Only works with **trivially destructible types** to ensure safe byte-level wiping.

### Constraint: Trivially Destructible Types

To guarantee that memory can be safely wiped at the byte level, the allocator can **only be used with trivially destructible types**. A `static_assert` will trigger a compile-time error otherwise.

This includes fundamental types (`char`, `uint8_t`, `int`) and simple structs (POD-like types).

---

## When to Use `secure_allocator<T>`

Use `secure_allocator<T>` when handling any **security-critical** data that:

* Should **never be written to disk** (even by OS swapping)
* Must be **wiped from RAM** after use
* Could be a target of memory-based attacks (keys, passwords, session tokens, etc.)

Typical scenarios include:

* Cryptographic key storage
* Secure vectors for in-memory passwords or tokens
* Temporary buffers for encryption/decryption routines
* Private configuration fields

---

## Example

### Define a secure container:

```cpp
using secure_buffer = std::vector<uint8_t, neonfs::secure_allocator<uint8_t>>;
```

### Use it like a normal vector:

```cpp
secure_buffer key(32);  // 32-byte AES key
key[0] = 0x13;
// key data lives in secure memory
```

Upon destruction or manual deallocation, the memory will be securely wiped and freed.

---

## Design Constraints and Notes

* Designed only for **POD-like**, trivially destructible types.
* Compatible with C++17 and later STL containers with conforming allocator APIs.
* Memory is aligned and zeroed before being released, but OpenSSL handles low-level secure memory details.
* For full STL compatibility, make sure `allocate()` and `deallocate()` are instance (not static) methods in your implementation.

---

For usage patterns and container integration, see the [Secure Allocator Usage](SecureAllocatorUsage.md) documentation.