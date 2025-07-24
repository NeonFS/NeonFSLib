#include <NeonFS/security/aes_encryption_provider.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

neonfs::security::AESEncryptionProvider::AESEncryptionProvider(const secure_bytes &&master_key, const size_t poolMaxSize = 5): contextPool_(AESGCMCtxPool::create(poolMaxSize)), key_(master_key) {
    if (key_.size() != 32) throw std::invalid_argument("Key must be 256 bits (32 bytes).");
}

neonfs::Result<neonfs::secure_bytes> neonfs::security::AESEncryptionProvider::encrypt(const secure_bytes &plain, secure_bytes &outIV, secure_bytes &outTag) {
    // Validate key first (most critical check)
    if (key_.size() != 32) {
        return Result<secure_bytes>::err(
            "Invalid key size: expected 32 bytes, got " +
            std::to_string(key_.size()));
    }

    // Auto-generate IV if empty
    if (outIV.empty()) {
        outIV.resize(iv_size());
        if (RAND_bytes(outIV.data(), outIV.size()) != 1) {
            return Result<secure_bytes>::err("Failed to generate secure IV");
        }
    }
    else if (outIV.size() != iv_size()) {
        return Result<secure_bytes>::err(
            "Invalid IV size: expected " + std::to_string(iv_size()) +
            " bytes, got " + std::to_string(outIV.size()));
    }

    // Prepare tag buffer (always overwrite)
    outTag.resize(tag_size());
    std::fill(outTag.begin(), outTag.end(), 0); // Clear any existing data

    const AESGCMCtxPool::Handle ctx_handle = contextPool_->acquire();

    // Initialize the encryption operation
    if (1 != EVP_EncryptInit_ex(ctx_handle->get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr)) {
        return Result<secure_bytes>::err("Failed to initialize AES-GCM encryption.");
    }

    // Set the IV length
    if (1 != EVP_CIPHER_CTX_ctrl(ctx_handle->get(), EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(outIV.size()), nullptr)) {
        return Result<secure_bytes>::err("Failed to set IV length.");
    }

    // Initialize key and IV
    if (1 != EVP_EncryptInit_ex(ctx_handle->get(), nullptr, nullptr, key_.data(), outIV.data())) {
        return Result<secure_bytes>::err("Failed to set key and IV.");
    }

    // Encrypt the data
    secure_bytes ciphertext(plain.size() + EVP_MAX_BLOCK_LENGTH); // Allocate enough space
    int len = 0;
    int ciphertext_len = 0;

    // Encrypt the plaintext
    if (1 != EVP_EncryptUpdate(ctx_handle->get(), ciphertext.data(), &len, plain.data(), static_cast<int>(plain.size()))) {
        return Result<secure_bytes>::err("Encryption failed during EVP_EncryptUpdate.");
    }
    ciphertext_len = len;

    // Finalize encryption
    if (1 != EVP_EncryptFinal_ex(ctx_handle->get(), ciphertext.data() + len, &len)) {
        return Result<secure_bytes>::err("Encryption failed during EVP_EncryptFinal_ex.");
    }
    ciphertext_len += len;

    // Verify ciphertext size matches plaintext size
    if (ciphertext_len != static_cast<int>(plain.size())) {
        return Result<secure_bytes>::err("Ciphertext size does not match plaintext size.");
    }

    // Get the authentication tag
    if (1 != EVP_CIPHER_CTX_ctrl(ctx_handle->get(), EVP_CTRL_GCM_GET_TAG, static_cast<int>(outTag.size()), outTag.data())) {
        return Result<secure_bytes>::err("Failed to retrieve authentication tag.");
    }

    // Resize the ciphertext to the actual size
    ciphertext.resize(ciphertext_len);

    return Result<secure_bytes>::ok(ciphertext);
}

neonfs::Result<neonfs::secure_bytes> neonfs::security::AESEncryptionProvider::decrypt(const secure_bytes &cipher, const secure_bytes &iv, secure_bytes &tag) {
    // Validate all inputs before processing
    if (key_.size() != 32) {
        return Result<secure_bytes>::err(
            "Invalid key size: expected 32 bytes, got " +
            std::to_string(key_.size()));
    }
    if (iv.empty() || iv.size() != iv_size()) {
        return Result<secure_bytes>::err(
            "Invalid IV: must be exactly " + std::to_string(iv_size()) +
            " bytes");
    }
    if (tag.empty() || tag.size() != tag_size()) {
        return Result<secure_bytes>::err(
            "Invalid tag: must be exactly " + std::to_string(tag_size()) +
            " bytes");
    }

    if (cipher.empty()) {
        return Result<secure_bytes>::err("Ciphertext cannot be empty");
    }

    const AESGCMCtxPool::Handle ctx_handle = contextPool_->acquire();

    secure_bytes plaintext(cipher.size());
    int len = 0, plaintext_len = 0;

    // Initialize decryption
    if (1 != EVP_DecryptInit_ex(ctx_handle->get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr)) {
        return Result<secure_bytes>::err("Failed to initialize AES-GCM decryption.");
    }

    // Set IV length
    if (1 != EVP_CIPHER_CTX_ctrl(ctx_handle->get(), EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(iv.size()), nullptr)) {
        return Result<secure_bytes>::err("Failed to set IV length.");
    }

    // Set key and IV
    if (1 != EVP_DecryptInit_ex(ctx_handle->get(), nullptr, nullptr, key_.data(), iv.data())) {
        return Result<secure_bytes>::err("Failed to set key/IV.");
    }

    // Decrypt the ciphertext
    if (1 != EVP_DecryptUpdate(ctx_handle->get(), plaintext.data(), &len, cipher.data(), static_cast<int>(cipher.size()))) {
        throw Result<secure_bytes>::err("Decryption failed during EVP_DecryptUpdate.");
    }
    plaintext_len = len;

    // Set the expected authentication tag
    if (1 != EVP_CIPHER_CTX_ctrl(ctx_handle->get(), EVP_CTRL_GCM_SET_TAG, static_cast<int>(tag.size()), tag.data())) {
        return Result<secure_bytes>::err("Failed to set authentication tag.");
    }

    // Finalize decryption and verify the tag
    const int ret = EVP_DecryptFinal_ex(ctx_handle->get(), plaintext.data() + len, &len);
    plaintext_len += len;

    // Check if decryption and tag verification were successful
    if (ret <= 0) {
        return Result<secure_bytes>::err("Decryption failed: Invalid tag or corrupted data.");
    }

    // Resize the plaintext to the actual size
    plaintext.resize(plaintext_len);

    return Result<secure_bytes>::ok(plaintext);
}

size_t neonfs::security::AESEncryptionProvider::iv_size() const {
    return 12;
}

size_t neonfs::security::AESEncryptionProvider::tag_size() const {
    return 16;
}