#include <gtest/gtest.h>
#include <NeonFS/security/aes_encryption_provider.h>
#include <NeonFS/core/types.h>
#include <thread>
#include <future>
#include <openssl/rand.h>
#include <atomic>
#include <chrono>
#include <mutex>

using namespace neonfs;
using namespace neonfs::security;

int main(int argc, char** argv) {
    initialize_secure_heap(64 * 1024 * 1024);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class AESEncryptionProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Generate a random 256-bit key
        secure_bytes key(32);
        RAND_bytes(key.data(), key.size());
        provider = std::make_unique<AESEncryptionProvider>(std::move(key), 6);
    }

    std::unique_ptr<AESEncryptionProvider> provider;
    secure_bytes testData = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};
};

// Basic Functionality
TEST_F(AESEncryptionProviderTest, EncryptDecryptRoundtrip) {
    secure_bytes iv, tag;
    auto encryptResult = provider->encrypt(testData, iv, tag);
    ASSERT_TRUE(encryptResult.is_ok());

    auto decryptResult = provider->decrypt(encryptResult.unwrap(), iv, tag);
    ASSERT_TRUE(decryptResult.is_ok());
    EXPECT_EQ(decryptResult.unwrap(), testData);
}

TEST_F(AESEncryptionProviderTest, DifferentIVEachTime) {
    secure_bytes iv1, iv2, tag1, tag2;
    provider->encrypt(testData, iv1, tag1);
    provider->encrypt(testData, iv2, tag2);

    EXPECT_NE(iv1, iv2) << "IVs must be unique for each encryption";
    EXPECT_NE(tag1, tag2) << "Tags must be unique for each encryption";
}

TEST_F(AESEncryptionProviderTest, TamperDetection) {
    secure_bytes iv, tag;
    auto cipher = provider->encrypt(testData, iv, tag).unwrap();

    // Tamper with ciphertext
    if (!cipher.empty()) cipher[0] ^= 0x01;

    auto result = provider->decrypt(cipher, iv, tag);
    EXPECT_TRUE(result.is_err()) << "Must detect tampered ciphertext";
}

TEST_F(AESEncryptionProviderTest, TamperedIVDetection) {
    secure_bytes iv, tag;
    auto cipher = provider->encrypt(testData, iv, tag).unwrap();

    // Tamper with IV
    if (!iv.empty()) iv[0] ^= 0x01;

    auto result = provider->decrypt(cipher, iv, tag);
    EXPECT_TRUE(result.is_err()) << "Must detect tampered IV";
}

TEST_F(AESEncryptionProviderTest, TamperedTagDetection) {
    secure_bytes iv, tag;
    auto cipher = provider->encrypt(testData, iv, tag).unwrap();

    // Tamper with tag
    if (!tag.empty()) tag[0] ^= 0x01;

    auto result = provider->decrypt(cipher, iv, tag);
    EXPECT_TRUE(result.is_err()) << "Must detect tampered tag";
}

// Edge Cases
TEST_F(AESEncryptionProviderTest, EmptyPlaintext) {
    secure_bytes iv, tag;
    auto result = provider->encrypt({}, iv, tag);
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(result.unwrap().empty());
}

TEST_F(AESEncryptionProviderTest, LargeData) {
    std::cout << "before test scope heap usage: " << CRYPTO_secure_used() << "\n";
    {
        std::cout << "begin of test scope Secure heap status:\n" << "  Used: " << CRYPTO_secure_used() << " bytes\n";
        secure_bytes largeData(5 * 1024 * 1024, 0x42); // 10MB of data
        secure_bytes iv, tag;
        std::cout << "after alocating vars Secure heap status:\n" << "  Used: " << CRYPTO_secure_used() << " bytes\n";

        auto encryptResult = provider->encrypt(largeData, iv, tag);
        std::cout << "after encrypt Secure heap status:\n" << "  Used: " << CRYPTO_secure_used() << " bytes\n";

        ASSERT_TRUE(encryptResult.is_ok());

        auto decryptResult = provider->decrypt(encryptResult.unwrap(), iv, tag);
        std::cout << "after decrypt Secure heap status:\n" << "  Used: " << CRYPTO_secure_used() << " bytes\n";
        ASSERT_TRUE(decryptResult.is_ok());
        EXPECT_EQ(decryptResult.unwrap(), largeData);
        std::cout << "end of test scope Secure heap status:\n" << "  Used: " << CRYPTO_secure_used() << " bytes\n";
    }
    std::cout << "after test scope heap usage: " << CRYPTO_secure_used() << "\n";
}

// Thread Safety
TEST_F(AESEncryptionProviderTest, ThreadSafety) {
    constexpr int kThreads = 10;
    constexpr int kIterations = 100;

    std::vector<std::future<void>> futures;
    secure_bytes iv, tag;
    auto cipher = provider->encrypt(testData, iv, tag).unwrap();

    for (int i = 0; i < kThreads; ++i) {
        futures.push_back(std::async(std::launch::async, [&] {
            for (int j = 0; j < kIterations; ++j) {
                auto result = provider->decrypt(cipher, iv, tag);
                ASSERT_TRUE(result.is_ok());
                EXPECT_EQ(result.unwrap(), testData);
            }
        }));
    }

    for (auto& f : futures) {
        f.get();
    }
}

// Configuration
TEST_F(AESEncryptionProviderTest, ValidKeySizes) {
    secure_bytes keys[] = {
        secure_bytes(16), // Should fail - needs 32 bytes
        secure_bytes(32), // Valid
        secure_bytes(64)  // Should fail
    };

    for (auto& key : keys) {
        RAND_bytes(key.data(), key.size());
        if (key.size() != 32) {
            EXPECT_THROW(AESEncryptionProvider(std::move(key), 5), std::invalid_argument);
        } else {
            EXPECT_NO_THROW(AESEncryptionProvider(std::move(key), 5));
        }
    }
}

TEST_F(AESEncryptionProviderTest, ConstantSizes) {
    EXPECT_EQ(provider->iv_size(), 12); // Standard GCM IV size
    EXPECT_EQ(provider->tag_size(), 16); // Standard GCM tag size
}


TEST_F(AESEncryptionProviderTest, ParallelLargeDataEncryption) {
    using namespace neonfs;
    const size_t TOTAL_SIZE = 100 * 1024 * 1024; // 100MB
    const size_t CHUNK_SIZE = 512 * 1024;         // 512KB chunks
    const size_t NUM_THREADS = std::thread::hardware_concurrency();
    size_t secureHeapPeak = CRYPTO_secure_used();

    // 1. Create source data (100MB)
    std::vector<uint8_t> sourceData(TOTAL_SIZE, 0x42);

    // 2. Prepare encrypted output (pre-allocate for thread safety)
    std::vector<uint8_t> encryptedData(TOTAL_SIZE);
    std::mutex encryptionMutex;

    // 3. Performance tracking
    std::atomic<size_t> bytesProcessed{0};
    auto startTime = std::chrono::high_resolution_clock::now();

    // 4. Parallel processing function
    auto processChunk = [&](const size_t start, const size_t end) {
        secure_bytes iv(12), tag(16);
        size_t localProcessed = 0;

        for (size_t offset = start; offset < end; offset += CHUNK_SIZE) {
            size_t currentChunkSize = std::min(CHUNK_SIZE, end - offset);

            // Create secure chunk
            secure_bytes chunk(
                sourceData.begin() + offset,
                sourceData.begin() + offset + currentChunkSize
            );

            // Encrypt
            auto cipher = provider->encrypt(chunk, iv, tag).unwrap();

            // Store result (thread-safe)
            {
                std::lock_guard<std::mutex> lock(encryptionMutex);
                secureHeapPeak = CRYPTO_secure_used() > secureHeapPeak ? CRYPTO_secure_used() : secureHeapPeak;
                std::copy(cipher.begin(), cipher.end(),
                         encryptedData.begin() + offset);
            }

            localProcessed += currentChunkSize;
        }

        bytesProcessed += localProcessed;
    };

    // 5. Launch threads
    std::vector<std::thread> threads;
    size_t chunkPerThread = TOTAL_SIZE / NUM_THREADS;

    for (size_t i = 0; i < NUM_THREADS; ++i) {
        size_t start = i * chunkPerThread;
        size_t end = (i == NUM_THREADS - 1) ? TOTAL_SIZE : start + chunkPerThread;
        threads.emplace_back(processChunk, start, end);
    }

    // 6. Wait for completion
    for (auto& t : threads) {
        t.join();
    }

    // 7. Calculate performance
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();

    double speedMBps = (bytesProcessed / (1024.0 * 1024.0)) /
                      (duration / 1000.0);

    std::cout << "Parallel encryption completed:\n"
              << "  Total size: " << TOTAL_SIZE << " bytes\n"
              << "  Chunk size: " << CHUNK_SIZE << " bytes\n"
              << "  Threads: " << NUM_THREADS << "\n"
              << "  Chunks: " << (TOTAL_SIZE / CHUNK_SIZE) << "\n"
              << "  Time: " << duration << " ms\n"
              << "  Speed: " << speedMBps << " MB/s\n"
              << "  Secure heap peak: " << secureHeapPeak << " bytes\n";

    // 8. Verify output size
    EXPECT_EQ(encryptedData.size(), TOTAL_SIZE);
}