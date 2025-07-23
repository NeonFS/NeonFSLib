#include <gtest/gtest.h>
#include <NeonFS/security/aes_gcm_ctx_pool.h>
#include <vector>
#include <thread>
#include <future>

using namespace neonfs::security;

class AESGCMCtxPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool = std::make_shared<AESGCMCtxPool>(3); // Small pool for testing
    }

    std::shared_ptr<AESGCMCtxPool> pool;
};

TEST_F(AESGCMCtxPoolTest, BasicAcquireRelease) {
    {
        auto handle = pool->acquire();
        EXPECT_NE(handle.operator->(), nullptr);
        EXPECT_NO_THROW(handle->reset());
    } // handle released here
}

TEST_F(AESGCMCtxPoolTest, PoolReusesObjects) {
    AESGCMCtx* firstAddress = nullptr;
    {
        auto handle = pool->acquire();
        firstAddress = handle.operator->();
    }

    {
        auto handle = pool->acquire();
        EXPECT_EQ(handle.operator->(), firstAddress);
    }
}

TEST_F(AESGCMCtxPoolTest, StrictPoolSizeEnforcement) {
    // Pool size = 3 (from SetUp)
    auto h1 = pool->acquire();
    auto h2 = pool->acquire();
    auto h3 = pool->acquire();  // Pool is now exhausted

    // Verify we can't create a 4th context
    std::atomic<bool> fourth_acquired{false};
    auto future = std::async(std::launch::async, [this, &fourth_acquired] {
        auto h4 = pool->acquire();  // This should block
        fourth_acquired = true;
    });

    // Give it a moment to try (but not the full 10ms!)
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    EXPECT_FALSE(fourth_acquired.load()) << "Pool should block when full";

    // Release one context
    h1.reset();  // Returns to pool

    // Now the fourth acquire should succeed quickly
    future.wait_for(std::chrono::milliseconds(1));
    EXPECT_TRUE(fourth_acquired.load()) << "Should acquire after release";

    // Cleanup
    future.get();  // Wait for thread completion
}

TEST_F(AESGCMCtxPoolTest, HandleDereferencing) {
    auto handle = pool->acquire();

    // Test both dereference operators
    EXPECT_NO_THROW((*handle).reset());
    EXPECT_NO_THROW(handle->reset());
}

TEST_F(AESGCMCtxPoolTest, ContextIsResetOnRelease) {
    uint8_t key[32] = {0};
    uint8_t iv[12] = {0};

    AESGCMCtx* ctxAddress = nullptr;
    {
        auto handle = pool->acquire();
        ctxAddress = handle.operator->();
        EXPECT_TRUE(handle->init(key, iv, sizeof(iv), true).is_ok());
    }

    {
        auto handle = pool->acquire();
        EXPECT_EQ(handle.operator->(), ctxAddress);
        // Should be reset, so init should work again
        EXPECT_TRUE(handle->init(key, iv, sizeof(iv), true).is_ok());
    }
}

TEST_F(AESGCMCtxPoolTest, ThreadSafety) {
    constexpr  int kThreads = 10;
    constexpr int kIterations = 100;

    std::vector<std::future<void>> futures;

    for (int i = 0; i < kThreads; ++i) {
        futures.push_back(std::async(std::launch::async, [this, kIterations] {
            for (int j = 0; j < kIterations; ++j) {
                auto handle = pool->acquire();
                EXPECT_NE(handle.operator->(), nullptr);
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }));
    }

    for (auto& f : futures) {
        f.get();
    }

    // Verify all objects were returned by acquiring maxPoolSize items
    std::vector<AESGCMCtxPool::Handle> handles;
    for (size_t i = 0; i < 3; ++i) {
        handles.push_back(pool->acquire());
    }

    // Next acquire should block (verify pool was full)
    std::atomic<bool> acquired{false};
    auto future = std::async(std::launch::async, [this, &acquired] {
        auto h = pool->acquire();
        acquired = true;
    });

    EXPECT_FALSE(acquired.load());

    // Cleanup
    handles.clear();
    future.wait_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(acquired.load());
}

TEST_F(AESGCMCtxPoolTest, BlocksWhenPoolExhausted) {
    // Take all available objects
    auto h1 = pool->acquire();
    auto h2 = pool->acquire();
    auto h3 = pool->acquire();

    bool acquired = false;
    auto future = std::async(std::launch::async, [this, &acquired] {
        auto h = pool->acquire(); // Should block
        acquired = true;
    });

    // Verify not acquired immediately
    EXPECT_FALSE(acquired);

    // Release one object
    h1.reset();  // Clear the handle

    // Should now be able to acquire
    future.wait_for(std::chrono::seconds(1));
    EXPECT_TRUE(acquired);
}

TEST_F(AESGCMCtxPoolTest, HandleMoveSemantics) {
    auto handle1 = pool->acquire();
    auto* ctx = handle1.operator->();

    // Move constructor
    auto handle2 = std::move(handle1);
    EXPECT_EQ(handle2.operator->(), ctx);
    EXPECT_EQ(handle1.operator->(), nullptr);

    // Move assignment
    auto handle3 = pool->acquire();
    auto* ctx3 = handle3.operator->();
    handle3 = std::move(handle2);
    EXPECT_EQ(handle3.operator->(), ctx);
    EXPECT_EQ(handle2.operator->(), nullptr);

    // ctx3 should have been returned to pool
    auto handle4 = pool->acquire();
    EXPECT_TRUE(handle4.operator->() == ctx3 || handle4.operator->() == ctx);
}

TEST_F(AESGCMCtxPoolTest, PerformanceBenchmark) {
    constexpr int kIterations = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < kIterations; ++i) {
        auto handle = pool->acquire();
        // Minimal work
    }

    auto duration = std::chrono::high_resolution_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    std::cout << kIterations << " iterations took " << ms << "ms" << std::endl;
}

TEST_F(AESGCMCtxPoolTest, ExceptionSafety) {
    try {
        auto handle = pool->acquire();
        throw std::runtime_error("Simulated error");
    }
    catch (...) {
        // Verify pool is in valid state after exception
        EXPECT_NO_THROW(pool->acquire());
    }
}