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

## Heap Initialization and Management

**WARNING:** The `secure_allocator` **requires** the OpenSSL secure heap to be initialized before it can be used. All allocations will fail until the heap is created.

You must call `neonfs::initialize_secure_heap()` once at application startup.

### Secure-heap memory usage

OpenSSL’s secure heap allocates in slab-like blocks (observed slabs: 512 KiB, 1 MiB, 2 MiB, 4 MiB, 8 MiB, 16 MiB, …). This means requested secure allocations are rounded up to the next slab.

#### Observed behaviour

On our test* platform the allocator rounds up to MiB-power buckets (plus a small bookkeeping overhead, ≈ 64 bytes):

| Plaintext size | Plain bytes | Secure heap delta | delta / plain |  
|---:|---:|------------------:|---:|  
| 512 KiB | 524,288 |           524,352 | 1.000 |  
| 768 KiB | 786,432 |         1,048,640 | 1.333 |  
| 1152 KiB | 1,179,648|         2,097,216 | 1.778 |  
| 1728 KiB | 1,769,472|         2,097,216 | 1.185 |  
| 2592 KiB | 2,654,208|         4,194,368 | 1.580 |  
| 3888 KiB | 3,981,312|         4,194,368 | 1.054 |  
| 5832 KiB | 5,971,968|         8,388,672 | 1.405 |  
| 8748 KiB | 8,957,952|        16,777,280 | 1.873 |  
| 13122 KiB|13,436,928|        16,777,280 | 1.249 |  
| 19683 KiB|20,155,392|        33,554,496 | 1.665 |

**Takeaway**: secure heap allocations round up to a slab size (e.g. 8,388,608 bytes for a 5 MiB request). The observed delta / plain ratio jumps because of rounding rather than a fixed multiplicative overhead.

**Takeaway**: small differences in requested N can cause large jumps in secure-heap usage. Choose chunk sizes that sit exactly at bucket boundaries (0.5 MiB, 1 MiB, 2 MiB, 4 MiB, …) to avoid rounding waste.

*see the `SecureMemoryAllocationPattern` in the `SecureAllocatorTest` provided in `tests/core/secure_allocator_tests.cpp`

#### Per-allocation overhead

Measure the per-allocation bookkeeping overhead by subtracting the bucket value from `CRYPTO_secure_used()` delta. On our platform it is ≈ 64 bytes per allocation. Include this when you want absolute `CRYPTO_secure_used` deltas.

#### Recommendations

- Pick chunk sizes that align with slab sizes:
  - Avoid awkward chunk sizes that round up to much larger slabs. For example, a 5 MiB chunk may round to 8 MiB; a 10 MiB chunk may round to 16 MiB. 
  - If memory is tight, use smaller chunk sizes (e.g., 512 KiB or 1 MiB) to avoid big rounding waste — but balance with throughput and syscall overhead.
- Estimate secure heap with the formula above and add safety margin (10–20%) for internal buffers, thread stack, or future OpenSSL behaviour changes.
- Always test and measure with your OpenSSL version and real workload; use the above formula + 10–20% margin if uncertain.

### Initialization
Place this call at the beginning of your `main` function:

```cpp
#include <NeonFS/core/types.h> // Or secure_allocator.hpp

int main() {
    try {
        // Initialize a 64MB secure heap. Choose a size that meets
        // your application's peak demand for secure memory.
        neonfs::initialize_secure_heap(64 * 1024 * 1024);
    } catch (const std::exception& e) {
        // Handle initialization failure
        return 1;
    }
    
    // ... rest of application ...
    
    return 0;
}
```

The size of the heap is a hard limit on the total amount of secure memory your application can have allocated at any one time. You must size it to accommodate the peak concurrent usage of all `secure_buffer` instances and other secure allocations.

### Cleanup
For a clean shutdown, you can optionally call `cleanup_secure_heap()` after all secure memory has been released. The function will throw an exception if any secure memory is still in use, helping to detect leaks.

```cpp
// At the end of your application
neonfs::cleanup_secure_heap();
```

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