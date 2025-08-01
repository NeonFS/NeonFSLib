# `AESGCMCtx` — RAII Wrapper for AES-GCM Operations

---
namespace:
- `neonfs::security`
---

## What is `AESGCMCtx`?

`AESGCMCtx` is a C++ RAII (Resource Acquisition Is Initialization) wrapper around OpenSSL's `EVP_CIPHER_CTX`. It is specifically designed to simplify the setup and management of contexts for **AES-256-GCM** encryption and decryption.

## Why Does It Exist?

The OpenSSL `EVP` API is powerful but complex and error-prone. `AESGCMCtx` solves several common problems:

* **Automatic Resource Management**: The constructor allocates the `EVP_CIPHER_CTX` and the destructor automatically frees it, preventing memory leaks.
* **Simplified Initialization**: It encapsulates the multi-step AES-GCM initialization logic into a single, clear `init()` method.
* **Error Safety**: The `init()` method returns a [`neonfs::Result<void>` object](../core/Result.md), allowing for clean, explicit error handling without relying on OpenSSL's integer return codes and error queue.
* **Reusability**: The context can be safely reset and reused for multiple operations, which is more efficient than creating and destroying contexts repeatedly.

## How Does It Work?

The class holds a pointer to an `EVP_CIPHER_CTX` and manages its lifecycle.

*   **Constructor**: `AESGCMCtx()` calls `EVP_CIPHER_CTX_new()` to create the context. It throws a `std::runtime_error` if allocation fails.
*   **Destructor**: `~AESGCMCtx()` calls `EVP_CIPHER_CTX_free()` to guarantee the context is released.
*   **`init()` method**: This is the core functional method. It configures the context for an AES-256-GCM operation by:
    1.  Resetting the context.
    2.  Setting the cipher type to `EVP_aes_256_gcm()`.
    3.  Setting the IV (Initialization Vector) length.
    4.  Providing the key and IV.
    5.  Setting the operation mode (encrypt or decrypt).

## API Reference

### `AESGCMCtx()`
Creates a new AES-GCM context. Throws if `EVP_CIPHER_CTX_new` fails.

### `~AESGCMCtx()`
Destroys the context and frees all associated memory.

### `Result<void> init(const uint8_t* key, const uint8_t* iv, int iv_len, bool encrypt)`
Initializes the context for an operation.
- **`key`**: A pointer to a 32-byte (256-bit) AES key.
- **`iv`**: A pointer to the initialization vector.
- **`iv_len`**: The length of the IV in bytes.
- **`encrypt`**: `true` for encryption, `false` for decryption.
- **Returns**: A `Result<void>` which is `ok()` on success or `err()` on failure.

### `EVP_CIPHER_CTX* get() const`
Provides direct access to the underlying `EVP_CIPHER_CTX*` pointer. This is needed to pass the context to OpenSSL's `EVP_EncryptUpdate`, `EVP_DecryptUpdate`, and other functions.

### `void reset() const`
Resets the context to a clean state, allowing it to be reused for a new operation without deallocating/reallocating memory.

---
For practical, complete code examples, see the [AESGCMCtx Usage Guide](AESGCMCtxUsage.md).