# Usage of `AESGCMCtx` for Encryption and Decryption

This document provides complete examples of how to use `AESGCMCtx` to perform AES-256-GCM encryption and decryption.

---

## Prerequisites

Include the necessary headers and ensure you have a 256-bit (32-byte) key. For this example, we will also use the `secure_bytes` type alias for buffers.
```cpp
#include <neonfs/security/aes_gcm_ctx.h>
#include <neonfs/types.h>
#include <neonfs/result.hpp>

// For OpenSSL functions
#include <openssl/evp.h>
#include <openssl/rand.h>

// A 256-bit key for the examples
const neonfs::secure_bytes key = { /* 32 bytes of a securely generated key */ };
```
---

## Encryption

AES-GCM encryption produces two outputs: the **ciphertext** and an **authentication tag**. Both are required for decryption. A common practice is to prepend the IV to the ciphertext and append the tag.

**Format:** `[IV (12 bytes)] + [Ciphertext] + [Tag (16 bytes)]`

### Complete Encryption Example

```cpp
Result<neonfs::secure_bytes> encrypt_data(const neonfs::secure_bytes& plaintext) {
using namespace neonfs;
using namespace neonfs::security;

    // 1. Setup IV and context
    secure_bytes iv(12); // GCM recommended IV size is 12 bytes
    if (RAND_bytes(iv.data(), iv.size()) != 1) {
        return Result<secure_bytes>::err("Failed to generate random IV");
    }

    AESGCMCtx ctx;
    auto init_res = ctx.init(key.data(), iv.data(), iv.size(), true); // true = encrypt
    if (init_res.is_err()) {
        return Result<secure_bytes>::err(init_res.unwrap_err());
    }

    // 2. Encrypt the plaintext
    secure_bytes ciphertext(plaintext.size());
    int len = 0;
    if (1 != EVP_EncryptUpdate(ctx.get(), ciphertext.data(), &len, plaintext.data(), plaintext.size())) {
        return Result<secure_bytes>::err("EVP_EncryptUpdate failed");
    }

    // 3. Finalize encryption
    int final_len = 0;
    if (1 != EVP_EncryptFinal_ex(ctx.get(), ciphertext.data() + len, &final_len)) {
        return Result<secure_bytes>::err("EVP_EncryptFinal_ex failed");
    }
    ciphertext.resize(len + final_len);

    // 4. Get the authentication tag
    secure_bytes tag(16);
    if (1 != EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, 16, tag.data())) {
        return Result<secure_bytes>::err("Failed to get GCM tag");
    }

    // 5. Assemble the final payload: IV + Ciphertext + Tag
    secure_bytes encrypted_payload;
    encrypted_payload.reserve(iv.size() + ciphertext.size() + tag.size());
    encrypted_payload.insert(encrypted_payload.end(), iv.begin(), iv.end());
    encrypted_payload.insert(encrypted_payload.end(), ciphertext.begin(), ciphertext.end());
    encrypted_payload.insert(encrypted_payload.end(), tag.begin(), tag.end());

    return Result<secure_bytes>::ok(std::move(encrypted_payload));
}
```
---

## Decryption

Decryption requires the ciphertext, IV, and authentication tag. The operation will only succeed if the tag is valid, which ensures the data is both authentic and unmodified.

### Complete Decryption Example

```cpp
Result<neonfs::secure_bytes> decrypt_data(const neonfs::secure_bytes& encrypted_payload) {
using namespace neonfs;
using namespace neonfs::security;

    // 1. Deconstruct the payload
    if (encrypted_payload.size() < 28) { // 12 (IV) + 16 (Tag)
        return Result<secure_bytes>::err("Invalid payload size");
    }
    const secure_bytes iv(encrypted_payload.begin(), encrypted_payload.begin() + 12);
    const secure_bytes tag(encrypted_payload.end() - 16, encrypted_payload.end());
    const secure_bytes ciphertext(encrypted_payload.begin() + 12, encrypted_payload.end() - 16);

    // 2. Setup context
    AESGCMCtx ctx;
    auto init_res = ctx.init(key.data(), iv.data(), iv.size(), false); // false = decrypt
    if (init_res.is_err()) {
        return Result<secure_bytes>::err(init_res.unwrap_err());
    }

    // 3. Provide the authentication tag
    if (1 != EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, 16, (void*)tag.data())) {
        return Result<secure_bytes>::err("Failed to set GCM tag");
    }

    // 4. Decrypt the ciphertext
    secure_bytes plaintext(ciphertext.size());
    int len = 0;
    if (1 != EVP_DecryptUpdate(ctx.get(), plaintext.data(), &len, ciphertext.data(), ciphertext.size())) {
        return Result<secure_bytes>::err("EVP_DecryptUpdate failed");
    }

    // 5. Finalize and authenticate
    // This is the critical step. It fails if the tag does not match.
    int final_len = 0;
    if (1 != EVP_DecryptFinal_ex(ctx.get(), plaintext.data() + len, &final_len)) {
        return Result<secure_bytes>::err("Decryption failed: authentication tag mismatch.");
    }
    plaintext.resize(len + final_len);

    return Result<secure_bytes>::ok(std::move(plaintext));
}
```

---

## Performance Note

For high-performance, multi-threaded applications, creating a new `AESGCMCtx` for every operation can be inefficient. Consider using a pool of contexts.

See [AESGCMCtxPoolUsage.md](AESGCMCtxPoolUsage.md) for more details.
