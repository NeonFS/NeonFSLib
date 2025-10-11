# Usage of `AESEncryptionProvider`

This document provides a practical guide on how to use the `AESEncryptionProvider` for secure and efficient data encryption and decryption.

---

## Setup

First, include the necessary header and prepare your master key. The provider is designed to be instantiated once and shared throughout your application to take advantage of its internal context pool.
```cpp
#include <NeonFS/security/aes_encryption_provider.h>
#include <NeonFS/core/types.h>
#include <memory>
#include <openssl/rand.h> // For key generation

// In a real application, this key should be loaded from a secure keystore.
// For this example, we generate a random one.
neonfs::secure_bytes get_master_key() {
    neonfs::secure_bytes key(32);
    if (RAND_bytes(key.data(), key.size()) != 1) {
        throw std::runtime_error("Failed to generate a random key");
    }
    return key;
}

// Create the provider instance. It should be long-lived.
// The pool size '8' allows 8 concurrent crypto operations.
auto provider = std::make_unique<neonfs::security::AESEncryptionProvider>(
    get_master_key(),
    8
);
```
---

## Basic Encryption & Decryption

The core workflow involves calling `encrypt` and then `decrypt`. The provider handles the generation of the IV for you.

### Complete Roundtrip Example
```cpp
void encrypt_decrypt_example(neonfs::security::AESEncryptionProvider* provider) {
    using namespace neonfs;

    // 1. Prepare your plaintext data in a secure buffer
    secure_bytes original_data = {'S', 'e', 'n', 's', 'i', 't', 'i', 'v', 'e', ' ', 'I', 'n', 'f', 'o'};

    // 2. Declare buffers for the outputs
    std::vector<uint8_t> iv;
    std::vector<uint8_t> tag;
    std::vector<uint8_t> ciphertext;

    // 3. Encrypt the data
    auto encrypt_result = provider->encrypt(original_data, iv, tag);
    if (encrypt_result.is_err()) {
        // Handle encryption error
        return;
    }
    ciphertext = encrypt_result.unwrap();

    // At this point, you would typically store or transmit the iv, tag, and ciphertext.

    // 4. Decrypt the data
    auto decrypt_result = provider->decrypt(ciphertext, iv, tag);
    if (decrypt_result.is_err()) {
        // Handle decryption error (e.g., tampered data)
        return;
    }
    secure_bytes decrypted_data = decrypt_result.unwrap();

    // 5. Verify the result
    assert(original_data == decrypted_data);
    std::cout << "Successfully encrypted and decrypted data!" << std::endl;
}
```
---

## Security: Tamper Detection

The `decrypt` method will fail if the ciphertext, IV, or tag have been modified. This is a critical feature of AES-GCM.
```cpp
void tamper_detection_example(neonfs::security::AESEncryptionProvider* provider) {
    using namespace neonfs;
    secure_bytes original_data = {1, 2, 3};
    std::vector<uint8_t> iv, tag;

    auto ciphertext = provider->encrypt(original_data, iv, tag).unwrap();

    // Tamper with the last byte of the ciphertext
    if (!ciphertext.empty()) {
        ciphertext.back() ^= 0xFF; 
    }

    auto decrypt_result = provider->decrypt(ciphertext, iv, tag);

    if (decrypt_result.is_err()) {
        // This block is expected to be hit
        std::cout << "Decryption failed as expected: " << decrypt_result.unwrap_err().message << std::endl;
    } else {
        std::cout << "Error: Tampered data was not detected!" << std::endl;
    }
}
```
---

## Multi-threading

The `AESEncryptionProvider` instance is thread-safe. You can share a single `std::unique_ptr` or `std::shared_ptr` to it across multiple threads without any external locking.
```cpp
#include <thread>
#include <vector>

void worker_func(neonfs::security::AESEncryptionProvider* provider) {
    // Each thread can call encrypt/decrypt concurrently
    // ... same logic as the roundtrip example ...
}

void multi_thread_example(std::unique_ptr<neonfs::security::AESEncryptionProvider>& provider) {
    std::vector<std::thread> threads;
    for(int i = 0; i < 10; ++i) {
        threads.emplace_back(worker_func, provider.get());
    }

    for(auto& t : threads) {
        t.join();
    }
}
```
---

## Important: Secure Heap and Memory Allocation

**WARNING**: The `AESEncryptionProvider` and all underlying cryptography components rely on a pre-allocated **secure memory arena** provided by OpenSSL. You are responsible for initializing this heap at application startup.

### Heap Initialization
Before any cryptographic operations, you must create the secure heap. Failure to do so will cause operations to fail.
```cpp
#include <NeonFS/core/types.h>

int main(int argc, char** argv) {
    // Initialize a 64MB secure heap. This must be done once.
    neonfs::initialize_secure_heap(64 * 1024 * 1024);

    // ... rest of your application's startup code ...
}
```