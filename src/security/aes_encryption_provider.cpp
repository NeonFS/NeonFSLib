#include <NeonFS/security/aes_encryption_provider.h>

#include <openssl/evp.h>

neonfs::security::AESEncryptionProvider::AESEncryptionProvider(const secure_vector<uint8_t> &&master_key, const size_t poolMaxSize = 5): contextPool_(poolMaxSize), key_(master_key) {}

neonfs::Result<neonfs::secure_vector<uint8_t>> neonfs::security::AESEncryptionProvider::encrypt(const secure_vector<uint8_t> &plain, secure_vector<uint8_t> &outIV, secure_vector<uint8_t> &outTag) {
    // Validate input sizes
    if (key_.size() != 32) return Result<secure_vector<uint8_t>>::err("Key must be 256 bits (32 bytes).");
    if (outIV.size() != 12) return Result<secure_vector<uint8_t>>::err("IV must be 96 bits (12 bytes).");
    if (outTag.size() != 16) return Result<secure_vector<uint8_t>>::err("Tag must be 128 bits (16 bytes).");

    const AESGCMCtxPool::Handle ctx_handle = contextPool_.acquire();

    // Initialize the encryption operation
    if (1 != EVP_EncryptInit_ex(ctx_handle->get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr)) {
        return Result<secure_vector<uint8_t>>::err("Failed to initialize AES-GCM encryption.");
    }

    // Set the IV length
    if (1 != EVP_CIPHER_CTX_ctrl(ctx_handle->get(), EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(outIV.size()), nullptr)) {
        return Result<secure_vector<uint8_t>>::err("Failed to set IV length.");
    }

    // Initialize key and IV
    if (1 != EVP_EncryptInit_ex(ctx_handle->get(), nullptr, nullptr, key_.data(), outIV.data())) {
        return Result<secure_vector<uint8_t>>::err("Failed to set key and IV.");
    }

    // Encrypt the data
    secure_vector<uint8_t> ciphertext(plain.size() + EVP_MAX_BLOCK_LENGTH); // Allocate enough space
    int len = 0;
    int ciphertext_len = 0;

    // Encrypt the plaintext
    if (1 != EVP_EncryptUpdate(ctx_handle->get(), ciphertext.data(), &len, plain.data(), static_cast<int>(plain.size()))) {
        return Result<secure_vector<uint8_t>>::err("Encryption failed during EVP_EncryptUpdate.");
    }
    ciphertext_len = len;

    // Finalize encryption
    if (1 != EVP_EncryptFinal_ex(ctx_handle->get(), ciphertext.data() + len, &len)) {
        return Result<secure_vector<uint8_t>>::err("Encryption failed during EVP_EncryptFinal_ex.");
    }
    ciphertext_len += len;

    // Verify ciphertext size matches plaintext size
    if (ciphertext_len != static_cast<int>(plain.size())) {
        return Result<secure_vector<uint8_t>>::err("Ciphertext size does not match plaintext size.");
    }

    // Get the authentication tag
    if (1 != EVP_CIPHER_CTX_ctrl(ctx_handle->get(), EVP_CTRL_GCM_GET_TAG, static_cast<int>(outTag.size()), outTag.data())) {
        return Result<secure_vector<uint8_t>>::err("Failed to retrieve authentication tag.");
    }

    // Resize the ciphertext to the actual size
    ciphertext.resize(ciphertext_len);

    return Result<secure_vector<uint8_t>>::ok(ciphertext);
}

neonfs::Result<neonfs::secure_vector<uint8_t>> neonfs::security::AESEncryptionProvider::decrypt(const secure_vector<uint8_t> &cipher, const secure_vector<uint8_t> &iv, secure_vector<uint8_t> &tag) {
    // Validate input sizes
    if (key_.size() != 32) return Result<secure_vector<uint8_t>>::err("Key must be 256 bits (32 bytes).");
    if (iv.size() != 12) return Result<secure_vector<uint8_t>>::err("IV must be 96 bits (12 bytes).");
    if (tag.size() != 16) return Result<secure_vector<uint8_t>>::err("Tag must be 128 bits (16 bytes).");

    const AESGCMCtxPool::Handle ctx_handle = contextPool_.acquire();

    secure_vector<uint8_t> plaintext(cipher.size());
    int len = 0, plaintext_len = 0;

    // Initialize decryption
    if (1 != EVP_DecryptInit_ex(ctx_handle->get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr)) {
        return Result<secure_vector<uint8_t>>::err("Failed to initialize AES-GCM decryption.");
    }

    // Set IV length
    if (1 != EVP_CIPHER_CTX_ctrl(ctx_handle->get(), EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(iv.size()), nullptr)) {
        return Result<secure_vector<uint8_t>>::err("Failed to set IV length.");
    }

    // Set key and IV
    if (1 != EVP_DecryptInit_ex(ctx_handle->get(), nullptr, nullptr, key_.data(), iv.data())) {
        return Result<secure_vector<uint8_t>>::err("Failed to set key/IV.");
    }

    // Decrypt the ciphertext
    if (1 != EVP_DecryptUpdate(ctx_handle->get(), plaintext.data(), &len, cipher.data(), static_cast<int>(cipher.size()))) {
        throw Result<secure_vector<uint8_t>>::err("Decryption failed during EVP_DecryptUpdate.");
    }
    plaintext_len = len;

    // Set the expected authentication tag
    if (1 != EVP_CIPHER_CTX_ctrl(ctx_handle->get(), EVP_CTRL_GCM_SET_TAG, static_cast<int>(tag.size()), tag.data())) {
        return Result<secure_vector<uint8_t>>::err("Failed to set authentication tag.");
    }

    // Finalize decryption and verify the tag
    const int ret = EVP_DecryptFinal_ex(ctx_handle->get(), plaintext.data() + len, &len);
    plaintext_len += len;

    // Check if decryption and tag verification were successful
    if (ret <= 0) {
        return Result<secure_vector<uint8_t>>::err("Decryption failed: Invalid tag or corrupted data.");
    }

    // Resize the plaintext to the actual size
    plaintext.resize(plaintext_len);

    return Result<secure_vector<uint8_t>>::ok(plaintext);
}

size_t neonfs::security::AESEncryptionProvider::iv_size() const {
    return 12;
}

size_t neonfs::security::AESEncryptionProvider::tag_size() const {
    return 16;
}