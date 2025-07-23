# `AESEncryptionProvider` — High-Level Encryption Service

---
namespace:
- `neonfs::security`
---

## Important: Secure Heap and Memory Allocation

**WARNING:** The `AESEncryptionProvider` and all underlying cryptography components rely on a pre-allocated **secure memory arena** provided by OpenSSL.

> If the secure heap is **not initialized before use**, OpenSSL's allocator may fall back to the system allocator, which **does not guarantee secure memory protections**.

However, **NeonFS’s custom [secure_allocator](../core/SecureAllocator.md) will detect uninitialized secure heap usage and throw an error or abort**, preventing insecure operation.

**Therefore, you must always initialize the secure heap at application startup** (e.g., via `initialize_secure_heap(...)`) to ensure:

- Cryptographic data is stored in protected memory,
- Sensitive buffers are zeroed on deallocation,

Failure to do so will cause allocation calls in `secure_allocator` to fail immediately.

### Heap Initialization
Before performing any cryptographic operation, initialize the secure heap exactly once, typically during application startup:

```cpp
#include <NeonFS/core/types.h>

int main(int argc, char** argv) {
    // Initialize a 64MB secure heap. This must be done once before cryptographic use.
    initialize_secure_heap(64 * 1024 * 1024); // Defined in NeonFS/core/secure_allocator.hpp

    // ... continue with your application's startup ...
}
```

### Memory Overhead
Cryptographic operations require significantly more memory than the size of the plaintext alone. A single `encrypt` or `decrypt` call needs space for the input data, the output data, and internal buffers.

*   An operation on data of size `N` requires heap space for the `N`-byte input and `N`-byte output simultaneously.
*   OpenSSL adds its own overhead, which can be around **1.7x** the size of the data being processed.
*   Therefore, a safe estimate for the total memory needed for one operation on data of size `N` is approximately **3.4x N**.

**You must initialize the secure heap with enough capacity to handle your largest concurrent workload.**

For example, if you plan to encrypt 10 MB data chunks across 4 threads concurrently, you should allocate at least:
`4 (threads) * 10 MB (chunk size) * 3.4 (overhead factor) = 136 MB`

### Final notes

* It’s best to round up when setting heap size.
* If you want to be even more conservative, consider a 3.5× or 4× multiplier.
* Always validate sizing with your own benchmarks and monitor `CRYPTO_secure_used()`.

## What is `AESEncryptionProvider`?

`AESEncryptionProvider` is a high-level, thread-safe service that simplifies AES-256-GCM encryption and decryption. It acts as a facade, abstracting away the complexities of key management, context pooling, and the underlying OpenSSL API.

It is designed to be the main entry point for all standard encryption and decryption tasks within the NeonFSLib.

## Why Does It Exist?

While `AESGCMCtx` and `AESGCMCtxPool` provide powerful building blocks, they still require the user to manage the OpenSSL workflow directly. The `AESEncryptionProvider` solves this by offering a much simpler, safer, and more opinionated interface.

*   **Ease of Use**: Provides two simple methods: `encrypt()` and `decrypt()`.
*   **Security by Default**:
    *   Enforces a 256-bit (32-byte) key size.
    *   Automatically generates a cryptographically secure 12-byte IV.
    *   Ensures a 16-byte authentication tag is always used.
    *   Uses `secure_bytes` for all sensitive data to leverage the secure heap.
*   **High Performance**: It transparently uses an internal `AESGCMCtxPool` to make concurrent operations highly efficient.
*   **Robust Error Handling**: All operations return a `neonfs::Result`, providing clear, non-exceptional error handling for common failures like tag mismatches (tamper detection).

## How Does It Work?

1.  **Initialization**: An instance is created with a 32-byte master key, which it stores securely. It also initializes an internal `AESGCMCtxPool` to a specified size.
2.  **Encryption (`encrypt`)**:
    *   It acquires an `AESGCMCtx` from its internal pool.
    *   It generates a secure, random 12-byte IV.
    *   It performs the AES-256-GCM encryption operation.
    *   It retrieves the 16-byte authentication tag.
    *   It returns the ciphertext and populates the IV and tag buffers provided by the caller.
3.  **Decryption (`decrypt`)**:
    *   It validates that the provided IV and tag have the correct sizes.
    *   It acquires an `AESGCMCtx` from the pool.
    *   It provides the key, IV, ciphertext, *and* tag to the context.
    *   It performs the decryption. The underlying OpenSSL function will only succeed if the tag is valid for the given key, IV, and ciphertext.
    *   If the tag is invalid, the operation fails, and an error `Result` is returned, preventing data tampering.

## API Reference

### `AESEncryptionProvider(const secure_bytes&& master_key, size_t poolMaxSize)`
The constructor for the provider.
- **`master_key`**: A `secure_bytes` buffer containing the 32-byte (256-bit) master key. The provider takes ownership of the key material. Throws `std::invalid_argument` if the key is not 32 bytes.
- **`poolMaxSize`**: The maximum number of `AESGCMCtx` objects to keep in the internal pool. This determines the maximum level of concurrency.

### `Result<secure_bytes> encrypt(const secure_bytes& plain, secure_bytes& outIV, secure_bytes& outTag)`
Encrypts plaintext data.
- **`plain`**: The plaintext to encrypt.
- **`outIV`**: A `secure_bytes` buffer that will be populated with the newly generated 12-byte IV.
- **`outTag`**: A `secure_bytes` buffer that will be populated with the 16-byte authentication tag.
- **Returns**: A `Result` containing the ciphertext on success, or an error on failure.

### `Result<secure_bytes> decrypt(const secure_bytes& cipher, const secure_bytes& iv, secure_bytes& tag)`
Decrypts ciphertext data and verifies its authenticity.
- **`cipher`**: The ciphertext to decrypt.
- **`iv`**: The 12-byte IV that was used during encryption.
- **`tag`**: The 16-byte authentication tag that was generated during encryption.
- **Returns**: A `Result` containing the plaintext on success. Returns an error if decryption fails for any reason, including an invalid authentication tag (tamper detection).

### `size_t iv_size() const`
Returns the required IV size (always 12).

### `size_t tag_size() const`
Returns the required tag size (always 16).

---
For complete, runnable examples, see the [AESEncryptionProvider Usage Guide](AESEncryptionProviderUsage.md).