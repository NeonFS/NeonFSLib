#pragma once
#include <NeonFS/core/interfaces.h>
#include <NeonFS/core/result.hpp>
#include <NeonFS/security/aes_gcm_ctx_pool.h>
#include <openssl/evp.h>

namespace neonfs::security {
    class AESEncryptionProvider final : public IEncryptionProvider {
        AESGCMCtxPool contextPool_;
        secure_vector<uint8_t> key_;
    public:
        // Enforce move-only master_key in constructor
        // explicit prevents accidental conversions (from other types like std::vector<uint8_t>).
        explicit AESEncryptionProvider(const secure_vector<uint8_t> &&master_key, const size_t poolMaxSize);

        Result<secure_vector<uint8_t>> encrypt(const secure_vector<uint8_t>& plain, secure_vector<uint8_t>& outIV, secure_vector<uint8_t>& outTag) override;
        Result<secure_vector<uint8_t>> decrypt(const secure_vector<uint8_t>& cipher, const secure_vector<uint8_t>& iv, secure_vector<uint8_t>& tag) override;

        size_t iv_size() const override;
        size_t tag_size() const override;
    };
} // namespace neon::security