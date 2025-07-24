#pragma once
#include <NeonFS/core/types.h>
#include <NeonFS/core/result.hpp>

namespace neonfs::security {
    using namespace neonfs;
    enum class KeyDerivationAlgorithm {
        PBKDF2_HMAC_SHA256,
        PBKDF2_HMAC_SHA512,
    };

    class KeyManager {
    public:
        /**
         * @brief Generates a master key of the specified size.
         *
         * This function generates a cryptographically secure random key with a default size of 32 bytes.
         * The key size must be greater than 0 and less than or equal to 512. If the key size is invalid
         * or the key generation fails, the function returns an error.
         *
         * @param size The size of the master key to generate, in bytes. Default size is 32.
         * @return A Result object containing the generated key as `secure_bytes` on success, or an error message on failure.
         */
        static Result<secure_bytes> generate_master_key(size_t size);

        /**
         * @brief Generates a cryptographically secure random salt of the specified size.
         *
         * This function creates a secure random salt with a default size of 16 bytes.
         * The size of the salt must be greater than 0 and less than or equal to 64.
         * If the provided size is invalid or if the random number generation process fails,
         * the function returns an error.
         *
         * @param size The size of the salt to generate, in bytes. Default size is 16.
         *             The size must be greater than 0 and less than or equal to 64.
         * @return A Result object containing the generated salt as `secure_bytes` on success,
         *         or an error message on failure.
         */
        static Result<secure_bytes> generate_salt(size_t size);

        /**
         * @brief Derives a cryptographic key from a password and salt using the specified key derivation algorithm.
         *
         * This function derives a key of the specified size using a password, a salt,
         * and a key derivation algorithm. It defaults to PBKDF2-HMAC-SHA256 with 100,000 iterations.
         * If the inputs are invalid (e.g., empty password or salt, or zero key size) or if
         * the key derivation process fails, an error is returned.
         *
         * @param password The input password used to derive the key.
         * @param salt A cryptographic salt used alongside the password for key derivation.
         * @param derived_key_size The size of the derived key in bytes.
         * @param algorithm The key derivation algorithm to use. Default is PBKDF2_HMAC_SHA256.
         * @param iterations The number of iterations for the key derivation algorithm. Default is 100,000.
         * @return A Result object containing the derived key as `secure_bytes` on success, or an error message on failure.
         */
        static Result<secure_bytes> derive_key(const secure_bytes& password, const secure_bytes& salt, size_t derived_key_size, KeyDerivationAlgorithm algorithm, unsigned iterations);

        /**
         * @brief Verifies a password by comparing its derived key to an expected value using a specified key derivation algorithm.
         *
         * This function takes a password and salt, derives a key of the specified size using the provided algorithm
         * and iterations, then performs a constant-time comparison against an expected derived key. The function securely
         * erases sensitive data (e.g., the derived key) from memory after usage. Errors are returned for invalid inputs
         * or derivation failures.
         *
         * @param password The input password to verify.
         * @param salt A cryptographic salt used together with the password for key derivation.
         * @param expected_derived_key The expected derived key to compare against.
         * @param derived_key_size The size of the derived key in bytes, must match the expected key size.
         * @param algorithm The key derivation algorithm to use, defaults to PBKDF2_HMAC_SHA256.
         * @param iterations The number of iterations for the key derivation algorithm, defaults to 100,000.
         * @return A Result object containing `true` if the password matches the expected derived key,
         *         `false` if it does not, or an error message on failure.
         */
        static Result<bool> verify_password(const secure_bytes& password, const secure_bytes& salt, const secure_bytes& expected_derived_key, size_t derived_key_size, KeyDerivationAlgorithm algorithm, unsigned iterations);

        // Prevent instantiation
        KeyManager() = delete;
        ~KeyManager() = delete;
        KeyManager(const KeyManager&) = delete;
        KeyManager& operator=(const KeyManager&) = delete;
    };

} // namespace neonfs::security