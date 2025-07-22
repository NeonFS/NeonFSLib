#pragma once
#include <NeonFS/core/result.hpp>
#include <openssl/evp.h>

namespace neonfs::security {
    class AESGCMCtx {
        EVP_CIPHER_CTX* ctx;

    public:
        AESGCMCtx();

        ~AESGCMCtx();

        EVP_CIPHER_CTX* get() const;

        void reset() const;

        // Initialize ctx for encryption or decryption
        Result<void> init(const uint8_t* key, const uint8_t* iv, int iv_len, bool encrypt) const;
    };
} // namespace neon::security