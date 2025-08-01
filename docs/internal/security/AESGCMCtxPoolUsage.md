# Usage of `AESGCMCtxPool`

This document provides a practical guide on how to use the `AESGCMCtxPool` to manage `AESGCMCtx` instances efficiently in a multi-threaded application.

---

## When to Use the Pool

You should use the `AESGCMCtxPool` when your application performs frequent AES-GCM encryption or decryption operations, especially across multiple threads.

*   **High-Throughput Servers**: Web servers or services that handle many concurrent TLS sessions or encrypted data streams.
*   **Parallel Processing**: Any application that processes encrypted data in parallel.

Using the pool avoids the significant overhead of creating and destroying an `EVP_CIPHER_CTX` for every single operation.

---

## Setup and Initialization

The pool is designed to be created once and shared across your application. The best way to manage its lifecycle is with a `std::shared_ptr`.

```cpp
#include <NeonFS/security/aes_gcm_ctx_pool.h>
#include <memory>

// Create a pool that can manage up to 8 concurrent contexts.
// Choose a size appropriate for your expected thread count.
auto crypto_pool = std::make_shared<neonfs::security::AESGCMCtxPool>(8);
```

This `crypto_pool` pointer should be passed to any threads or classes that need to perform cryptographic work.

---

## Encrypting Data with the Pool

The following example shows how to modify a function to acquire a context from the pool. The core encryption logic remains the same.

The key is to acquire a `handle` and use it like a pointer. The context is returned to the pool automatically when `handle` goes out of scope.

### Complete Encryption Example

```cpp
#include <NeonFS/security/aes_gcm_ctx_pool.h>
#include <NeonFS/core/types.h>
#include <NeonFS/core/result.hpp>
#include <openssl/evp.h>

// Assume 'key' and 'iv' are defined elsewhere
extern const neonfs::secure_bytes key;

// The function now takes a shared pointer to the pool
Result<neonfs::secure_bytes> encrypt_with_pool(
    std::shared_ptr<neonfs::security::AESGCMCtxPool> pool,
    const neonfs::secure_bytes& plaintext,
    const neonfs::secure_bytes& iv) 
{
    using namespace neonfs;
    using namespace neonfs::security;

    // 1. Acquire a context from the pool. This will block if the pool is exhausted.
    auto handle = pool->acquire();

    // 2. Use the handle to initialize the context.
    // Notice the use of operator->()
    auto init_res = handle->init(key.data(), iv.data(), iv.size(), true); // true = encrypt
    if (init_res.is_err()) {
        return Result<secure_bytes>::err(init_res.unwrap_err());
    }

    // The rest of the logic is identical to using a standalone AESGCMCtx
    secure_bytes ciphertext(plaintext.size());
    int len = 0;
    if (1 != EVP_EncryptUpdate(handle->get(), ciphertext.data(), &len, plaintext.data(), plaintext.size())) {
        return Result<secure_bytes>::err("EVP_EncryptUpdate failed");
    }

    int final_len = 0;
    if (1 != EVP_EncryptFinal_ex(handle->get(), ciphertext.data() + len, &final_len)) {
        return Result<secure_bytes>::err("EVP_EncryptFinal_ex failed");
    }
    ciphertext.resize(len + final_len);
    
    // ... get tag, assemble payload, etc. ...

    return Result<secure_bytes>::ok(std::move(ciphertext));
} // <-- handle goes out of scope here, and the context is automatically returned to the pool.
```

---

## Multi-threading Pattern

Here is a simplified example of how to use the pool from multiple threads.

```cpp
#include <thread>
#include <vector>

void worker_thread(std::shared_ptr<neonfs::security::AESGCMCtxPool> pool) {
    // Each thread gets its own data to process
    neonfs::secure_bytes plaintext = /* ... get some data ... */;
    neonfs::secure_bytes iv = /* ... generate a unique IV ... */;
    
    // The call to encrypt_with_pool will block only if all 8 contexts are in use.
    auto result = encrypt_with_pool(pool, plaintext, iv);
    
    if (result.is_ok()) {
        // ... handle success ...
    }
}

int main() {
    auto pool = std::make_shared<neonfs::security::AESGCMCtxPool>(8);
    std::vector<std::thread> threads;

    for (int i = 0; i < 20; ++i) {
        threads.emplace_back(worker_thread, pool);
    }

    for (auto& t : threads) {
        t.join();
    }

    return 0;
}

```
