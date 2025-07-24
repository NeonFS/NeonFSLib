# `KeyManager` — Cryptographic Key Operations

---
namespace:
- `neonfs::security`
---

## Overview

The `KeyManager` is a static utility class that provides a centralized, secure interface for fundamental cryptographic key and salt management tasks. It is designed to be a stateless, non-instantiable helper that ensures all operations are performed using best practices.

Its primary responsibilities include:
*   Generating cryptographically secure random keys and salts.
*   Deriving strong cryptographic keys from user passwords using the robust PBKDF2 algorithm.
*   Securely verifying user passwords against a stored derived key.

All sensitive data (passwords, keys, salts) is handled exclusively using `neonfs::secure_bytes` or `neonfs::secure_string` to ensure it resides in protected, zero-wiped memory (view [SecureAllocator](../core/SecureAllocator.md) for more details).

---

## Static Methods

The `KeyManager` class exposes its functionality through static methods and cannot be instantiated.

### `generate_master_key()`

Generates a high-entropy, cryptographically secure random key suitable for use as a master encryption key.

```cpp
static Result<secure_bytes> generate_master_key(size_t size = 32);
```

- **`size`**: The desired key size in bytes. Defaults to 32 bytes (256 bits). Must be between 1 and 512.
- **Returns**: A `Result` containing the `secure_bytes` key on success, or an error if generation fails.

### `generate_salt()`

Generates a cryptographically secure random salt, which is essential for password hashing.

```cpp
static Result<secure_bytes> generate_salt(size_t size = 16);
```

- **`size`**: The desired salt size in bytes. Defaults to 16 bytes (128 bits). Must be between 1 and 64.
- **Returns**: A `Result` containing the `secure_bytes` salt on success, or an error.

### `derive_key()`

Derives a strong cryptographic key from a lower-entropy input, such as a user's password. It uses the industry-standard **PBKDF2** algorithm.

```cpp
static Result<secure_bytes> derive_key(
    const secure_bytes& password,
    const secure_bytes& salt,
    size_t derived_key_size,
    KeyDerivationAlgorithm algorithm = KeyDerivationAlgorithm::PBKDF2_HMAC_SHA256,
    unsigned iterations = 100000
);
```

- **`password`**: The user's password.
- **`salt`**: The unique, random salt generated for this password.
- **`derived_key_size`**: The desired output key size in bytes.
- **`algorithm`**: The KDF hash function. Defaults to `PBKDF2_HMAC_SHA256`.
- **`iterations`**: The number of hashing rounds. Defaults to 100,000. Higher values are more secure but slower.
- **Returns**: A `Result` containing the derived key in `secure_bytes` on success, or an error.

### `verify_password()`

Securely checks if a given password matches a previously stored derived key.

```cpp
static Result<bool> verify_password(
    const secure_bytes& password,
    const secure_bytes& salt,
    const secure_bytes& expected_derived_key,
    size_t derived_key_size,
    KeyDerivationAlgorithm algorithm = KeyDerivationAlgorithm::PBKDF2_HMAC_SHA256,
    unsigned iterations = 100000
);
```

- **`password`**: The password to verify.
- **`salt`**: The salt that was used to create `expected_derived_key`.
- **`expected_derived_key`**: The stored key that was previously generated with `derive_key`.
- **`derived_key_size`**: The size of the key, which must match the size of `expected_derived_key`.
- **`algorithm`**, **`iterations`**: Must match the parameters used to create the original derived key.
- **Returns**: A `Result` containing `true` if the password is correct, `false` if not, or an error if the operation fails. The comparison is performed in **constant time** to prevent timing attacks.

---

## `KeyDerivationAlgorithm` Enum
Specifies the hash function to be used within the PBKDF2 algorithm.

| Value | Underlying Hash |
| --- | --- |
| `PBKDF2_HMAC_SHA256` | HMAC-SHA256 |
| `PBKDF2_HMAC_SHA512` | HMAC-SHA512 |

---

## Security Model

- **Stateless**: The manager holds no internal state.
- **Secure Memory**: All inputs and outputs containing sensitive data use `secure_bytes`, leveraging the `secure_allocator` to prevent secrets from being paged to disk and to ensure they are wiped after use.
- **Timing Attack Resistant**: `verify_password` uses a constant-time memory comparison (`CRYPTO_memcmp`) to prevent attackers from guessing a key based on comparison response times.

---

For practical, complete code examples, see the [KeyManager Usage Guide](KeyManagerUsage.md).