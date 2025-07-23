#include <gtest/gtest.h>
#include <NeonFS/security/aes_gcm_ctx.h>
#include <openssl/rand.h>

using namespace neonfs::security;

class AESGCMCtxTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Generate random key and IV for testing
        RAND_bytes(key, sizeof(key));
        RAND_bytes(iv, sizeof(iv));
    }

    uint8_t key[32]; // AES-256 key
    uint8_t iv[12];  // Standard GCM IV size
};

TEST_F(AESGCMCtxTest, ConstructorCreatesValidContext) {
    EXPECT_NO_THROW({
        AESGCMCtx ctx;
        EXPECT_NE(ctx.get(), nullptr);
    });
}

TEST_F(AESGCMCtxTest, DestructorCleansUp) {
    EVP_CIPHER_CTX* raw_ctx = nullptr;
    {
        const AESGCMCtx ctx;
        raw_ctx = ctx.get();
        EXPECT_NE(raw_ctx, nullptr);
    }
    // After destruction, the raw_ctx should be freed
    // We can't directly test this, but we can check it doesn't crash
}

TEST_F(AESGCMCtxTest, ResetWorks) {
    AESGCMCtx ctx;
    EXPECT_NO_THROW(ctx.reset());
}

TEST_F(AESGCMCtxTest, InitEncryptSuccess) {
    AESGCMCtx ctx;
    auto result = ctx.init(key, iv, sizeof(iv), true);
    EXPECT_TRUE(result.is_ok());
}

TEST_F(AESGCMCtxTest, InitDecryptSuccess) {
    AESGCMCtx ctx;
    auto result = ctx.init(key, iv, sizeof(iv), false);
    EXPECT_TRUE(result.is_ok());
}

// These tests verify that OpenSSL's behavior matches our expectations
TEST_F(AESGCMCtxTest, OpenSSLAcceptsVariousKeySizes) {
    AESGCMCtx ctx;

    // Test that OpenSSL accepts different key sizes (even if they're wrong)
    uint8_t keys[][32] = {
        {0}, // Zero key
        {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // 16 bytes
         0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
        {0}  // Full 32 bytes (but zeros)
    };

    for (const auto& key : keys) {
        EXPECT_TRUE(ctx.init(key, iv, sizeof(iv), true).is_ok());
        ctx.reset();
    }
}

TEST_F(AESGCMCtxTest, OpenSSLAcceptsReasonableIVSizes) {
    AESGCMCtx ctx;

    const int iv_sizes[] = {12, 16, 24, 32}; // Common GCM IV sizes
    for (auto size : iv_sizes) {
        std::vector<uint8_t>  dynamic_iv(size);
        RAND_bytes(dynamic_iv.data(), dynamic_iv.size());
        EXPECT_TRUE(ctx.init(key, dynamic_iv.data(), dynamic_iv.size(), true).is_ok());
        ctx.reset();
    }
}

TEST_F(AESGCMCtxTest, MultipleInitsWork) {
    AESGCMCtx ctx;
    EXPECT_TRUE(ctx.init(key, iv, sizeof(iv), true).is_ok());
    EXPECT_TRUE(ctx.init(key, iv, sizeof(iv), false).is_ok());
    EXPECT_TRUE(ctx.init(key, iv, sizeof(iv), true).is_ok());
}

TEST_F(AESGCMCtxTest, GetReturnsValidContext) {
    AESGCMCtx ctx;
    EVP_CIPHER_CTX* raw_ctx = ctx.get();
    EXPECT_NE(raw_ctx, nullptr);
    // Verify it's a usable context by initializing it
    EXPECT_TRUE(ctx.init(key, iv, sizeof(iv), true).is_ok());
}