# Usage of `KeyManager`

---

## Prerequisite: Heap Initialization

**CRITICAL:** The `KeyManager` relies heavily on `secure_bytes` and `secure_string` for all sensitive data. Before calling any `KeyManager` functions, you **must** initialize the secure heap.

Place this call at the very beginning of your `main()` function:

```cpp
#include <NeonFS/core/types.h>
#include <iostream>

int main() {
    try {
        // Initialize a secure heap large enough for your needs.
        neonfs::initialize_secure_heap(16 * 1024 * 1024); // 16 MB
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    // ... Application code ...
    
    return 0;
}
```

---

## Full Workflow: Password Registration and Verification

This is the most common use case: hashing a user's password for storage and later verifying it during login.

### 1. Registering a New User (Storing a Password)

Never store a password directly. Instead, derive a key from it and store the key and the salt.

```cpp
#include <NeonFS/security/key_manager.h>
#include <iostream>

// In a real app, these would be stored in a database user record.
neonfs::secure_bytes stored_salt;
neonfs::secure_bytes stored_derived_key;

void register_user(const neonfs::secure_string& password) {
    using namespace neonfs::security;

    // 1. Generate a unique, random salt for the new user.
    auto salt_res = KeyManager::generate_salt();
    if (salt_res.is_err()) {
        std::cerr << "Failed to generate salt: " << salt_res.unwrap_err().message << std::endl;
        return;
    }
    stored_salt = salt_res.unwrap();

    // 2. Derive a strong key from the password and salt.
    const size_t key_size = 32; // For AES-256
    auto key_res = KeyManager::derive_key(
        {password.begin(), password.end()}, // Convert secure_string to secure_bytes
        stored_salt,
        key_size
    );

    if (key_res.is_err()) {
        std::cerr << "Failed to derive key: " << key_res.unwrap_err().message << std::endl;
        return;
    }
    stored_derived_key = key_res.unwrap();

    // 3. Store `stored_salt` and `stored_derived_key`.
    //    The original password string should now be discarded.
    std::cout << "User registered successfully!" << std::endl;
}
```

### 2. Verifying a User (Login)

At login, you retrieve the user's salt and derived key, then use `verify_password` to check the provided password.

```cpp
void login_user(const neonfs::secure_string& attempted_password) {
    using namespace neonfs::security;

    // 1. Retrieve the user's stored salt and derived key from your database.
    //    (Here we use the globals from the registration example).
    
    // 2. Verify the password.
    auto verify_res = KeyManager::verify_password(
        {attempted_password.begin(), attempted_password.end()},
        stored_salt,
        stored_derived_key,
        stored_derived_key.size()
    );

    if (verify_res.is_err()) {
        std::cerr << "Verification process failed: " << verify_res.unwrap_err().message << std::endl;
        return;
    }

    // 3. Check the boolean result.
    if (verify_res.unwrap()) {
        std::cout << "Login successful!" << std::endl;
    } else {
        std::cout << "Login failed: Invalid password." << std::endl;
    }
}
```

---

## Generating a Standalone Master Key

If you need a high-entropy key for direct encryption (not derived from a password), use `generate_master_key`.

```cpp
#include <NeonFS/security/key_manager.h>

neonfs::secure_bytes get_new_encryption_key() {
    auto key_res = neonfs::security::KeyManager::generate_master_key(32); // 256-bit key
    if (key_res.is_err()) {
        throw std::runtime_error("Could not generate a master key!");
    }
    return key_res.unwrap();
}
```

---

## Best Practices

*   **Check Results:** Always check the `Result` object for errors after every `KeyManager` call.
*   **Store Salts and Derived Keys:** For password hashing, you must store both the unique salt and the resulting derived key for each user.
*   **Never Store Passwords:** The user's raw password should only exist in memory for the brief moment it's needed for derivation or verification. `secure_string` helps ensure it's wiped.
*   **Iteration Count:** The default of 100,000 iterations for `derive_key` is a strong baseline. For higher security needs, you can increase this value. Remember to use the same count for verification.