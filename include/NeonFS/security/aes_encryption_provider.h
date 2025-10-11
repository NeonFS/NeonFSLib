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
        explicit AESEncryptionProvider(secure_bytes &&master_key, size_t poolMaxSize);

        Result<std::vector<uint8_t>> encrypt(const secure_bytes& plain, std::vector<uint8_t>& outIV, std::vector<uint8_t>& outTag) override;
        Result<secure_bytes> decrypt(const std::vector<uint8_t>& cipher, const std::vector<uint8_t>& iv, std::vector<uint8_t>& tag) override;

        [[nodiscard]] size_t iv_size() const override;
        [[nodiscard]] size_t tag_size() const override;
    };
} // namespace neon::security