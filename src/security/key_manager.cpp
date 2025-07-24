#include <NeonFS/security/key_manager.h>
#include <openssl/rand.h>

neonfs::Result<neonfs::secure_bytes> neonfs::security::KeyManager::generate_master_key(const size_t size = 32) {
    if (size == 0 || size > 512) {
        return Result<secure_bytes>::err("Invalid key size");
    }

    secure_bytes key(size);
    if (RAND_bytes(key.data(), static_cast<int>(key.size())) != 1) {
        return Result<secure_bytes>::err("Failed to generate secure random key");
    }
    return Result<secure_bytes>::ok(key);
}

neonfs::Result<neonfs::secure_bytes> neonfs::security::KeyManager::generate_salt(const size_t size = 16) {
    if (size == 0 || size > 64) {
        return Result<secure_bytes>::err("Invalid salt size");
    }

    secure_bytes salt(size);
    if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1) {
        return Result<secure_bytes>::err("Failed to generate secure random salt");
    }
    return Result<secure_bytes>::ok(salt);
}

neonfs::Result<neonfs::secure_bytes> neonfs::security::KeyManager::derive_key(const secure_bytes &password, const secure_bytes &salt, size_t derived_key_size, const KeyDerivationAlgorithm algorithm = KeyDerivationAlgorithm::PBKDF2_HMAC_SHA256, unsigned iterations = 100000) {
    if (password.empty() || salt.empty() || derived_key_size == 0) {
        return Result<secure_bytes>::err("Invalid input parameters");
    }

    secure_bytes derived_key(derived_key_size);

    if (PKCS5_PBKDF2_HMAC(
        reinterpret_cast<const char *>(password.data()), static_cast<int>(password.size()),
        salt.data(), static_cast<int>(salt.size()),
        iterations,
        (algorithm == KeyDerivationAlgorithm::PBKDF2_HMAC_SHA256) ? EVP_sha256() : EVP_sha512(),
        static_cast<int>(derived_key.size()),
        derived_key.data()) != 1) {
        const std::string algo_name((algorithm == KeyDerivationAlgorithm::PBKDF2_HMAC_SHA256) ? "PBKDF2_HMAC_SHA256" : "PBKDF2_HMAC_SHA512");
        return Result<secure_bytes>::err("Key derivation failed (" + algo_name + ")");
    }

    return Result<secure_bytes>::ok(derived_key);
}

neonfs::Result<bool> neonfs::security::KeyManager::verify_password(const secure_bytes &password, const secure_bytes &salt, const secure_bytes &expected_derived_key, size_t derived_key_size, const KeyDerivationAlgorithm algorithm = KeyDerivationAlgorithm::PBKDF2_HMAC_SHA256, unsigned iterations = 100000) {
    // Input validation
    if (password.empty()) {
        return Result<bool>::err("Password cannot be empty");
    }
    if (salt.empty()) {
        return Result<bool>::err("Salt cannot be empty");
    }
    if (derived_key_size == 0 || derived_key_size > 64) {
        return Result<bool>::err("Invalid derived key size");
    }
    if (expected_derived_key.size() != derived_key_size) {
        return Result<bool>::err("Expected key size mismatch");
    }

    try {
        // Derive key from provided password and salt
        secure_bytes derived_key(derived_key_size);

        if (PKCS5_PBKDF2_HMAC(
            reinterpret_cast<const char *>(password.data()), static_cast<int>(password.size()),
            salt.data(), static_cast<int>(salt.size()),
            iterations,
            (algorithm == KeyDerivationAlgorithm::PBKDF2_HMAC_SHA256) ? EVP_sha256() : EVP_sha512(),
            static_cast<int>(derived_key_size),
            derived_key.data()) != 1) {
            return Result<bool>::err("Key derivation failed during verification");
        }

        // Constant-time comparison to prevent timing attacks
        bool matches = (CRYPTO_memcmp(
            derived_key.data(),
            expected_derived_key.data(),
            derived_key_size) == 0);

        // Securely wipe the derived key from memory
        OPENSSL_cleanse(derived_key.data(), derived_key.size());

        return Result<bool>::ok(matches);
    } catch (const std::exception& e) {
        return Result<bool>::err(std::string("Verification failed: ") + e.what());
    }
}
