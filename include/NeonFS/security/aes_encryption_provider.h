#pragma once
#include <NeonFS/core/interfaces.h>
#include <NeonFS/core/result.hpp>
#include <NeonFS/security/aes_gcm_ctx_pool.h>
#include <openssl/evp.h>

namespace neonfs::security {
    class AESEncryptionProvider final : public IEncryptionProvider {
        std::shared_ptr<AESGCMCtxPool> contextPool_;
        secure_bytes key_;
    public:
        // Enforce move-only master_key in constructor
        // explicit prevents accidental conversions (from other types like std::vector<uint8_t>).
        explicit AESEncryptionProvider(const secure_bytes &&master_key, const size_t poolMaxSize);

        Result<secure_bytes> encrypt(const secure_bytes& plain, secure_bytes& outIV, secure_bytes& outTag) override;
        Result<secure_bytes> decrypt(const secure_bytes& cipher, const secure_bytes& iv, secure_bytes& tag) override;

        size_t iv_size() const override;
        size_t tag_size() const override;
    };
} // namespace neon::security