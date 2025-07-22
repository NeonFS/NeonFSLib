#include <NeonFS/security/aes_gcm_ctx.h>

neonfs::security::AESGCMCtx::AESGCMCtx()  {
    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create EVP_CIPHER_CTX");
}

neonfs::security::AESGCMCtx::~AESGCMCtx() {
    if (ctx) EVP_CIPHER_CTX_free(ctx);
}

EVP_CIPHER_CTX *neonfs::security::AESGCMCtx::get() const {
    return ctx;
}

void neonfs::security::AESGCMCtx::reset() const {
    EVP_CIPHER_CTX_reset(ctx);
}

neonfs::Result<void> neonfs::security::AESGCMCtx::init(const uint8_t *key, const uint8_t *iv, const int iv_len, const bool encrypt) const {
    reset();
    if (1 != EVP_CipherInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr, encrypt ? 1 : 0))
        return Result<void>::err("Failed to initialize cipher");
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, nullptr))
        return Result<void>::err("Failed to set IV length");
    if (1 != EVP_CipherInit_ex(ctx, nullptr, nullptr, key, iv, encrypt ? 1 : 0))
        return Result<void>::err("Failed to initialize key/IV");
    return Result<void>::ok();
}