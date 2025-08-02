#include <gtest/gtest.h>
#include <NeonFS/core/types.h>
#include <NeonFS/storage/block_storage.h>
#include <filesystem>
#include <random>

namespace fs = std::filesystem;
using namespace neonfs::storage;

class BlockStorageTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary test file
        test_file = fs::temp_directory_path() / "block_storage_test.bin";
        config = {4096, 4096 * 100}; // 100 blocks of 4KB each
        BlockStorage::create(test_file.string(), config).unwrap();
    }

    void TearDown() override {
        if (fs::exists(test_file)) {
            fs::remove(test_file);
        }
    }

    fs::path test_file;
    neonfs::BlockStorageConfig config = {};
};

TEST_F(BlockStorageTest, CreateStorage) {
    // Test creation with invalid parameters
    EXPECT_TRUE(BlockStorage::create("", config).is_err());
    EXPECT_TRUE(BlockStorage::create("test.bin", {0, 4096}).is_err());
    EXPECT_TRUE(BlockStorage::create("test.bin", {512, 1000}).is_err()); // Not multiple

    // Test valid creation
    auto temp_file = fs::temp_directory_path() / "valid_create.bin";
    auto result = BlockStorage::create(temp_file.string(), {512, 512*10});
    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(fs::file_size(temp_file) == 512*10);
    fs::remove(temp_file);
}

TEST_F(BlockStorageTest, MountUnmount) {
    BlockStorage storage;

    // Test unmount when not mounted
    EXPECT_TRUE(storage.unmount().is_err());

    // Test mount with invalid path
    EXPECT_TRUE(storage.mount("", config).is_err());

    // Test valid mount
    EXPECT_TRUE(storage.mount(test_file.string(), config).is_ok());
    EXPECT_TRUE(storage.isMounted());

    // Test double mount
    EXPECT_TRUE(storage.mount(test_file.string(), config).is_err());

    // Test valid unmount
    EXPECT_TRUE(storage.unmount().is_ok());
    EXPECT_FALSE(storage.isMounted());
}

TEST_F(BlockStorageTest, ReadWriteOperations) {
    BlockStorage storage;
    storage.mount(test_file.string(), config).unwrap();

    // Test read invalid block
    EXPECT_TRUE(storage.readBlock(1000).is_err()); // Beyond block count

    // Test write invalid block
    std::vector<uint8_t> data(4096, 0xAA);
    EXPECT_TRUE(storage.writeBlock(1000, data).is_err());

    // Test write with invalid data size
    std::vector<uint8_t> small_data(100, 0xBB);
    auto a = storage.writeBlock(0, small_data);
    EXPECT_TRUE(a.is_ok()) << a.unwrap_err().message; // Should auto-pad
    std::vector<uint8_t> large_data(5000, 0xCC);
    EXPECT_TRUE(storage.writeBlock(0, large_data).is_err());

    // Test round-trip read/write
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 255);

    std::vector<uint8_t> test_data(4096);
    std::generate(test_data.begin(), test_data.end(), [&](){ return distrib(gen); });

    auto b = storage.writeBlock(5, test_data);
    EXPECT_TRUE(b.is_ok()) << b.unwrap_err().message;
    auto read_result = storage.readBlock(5);
    ASSERT_TRUE(read_result.is_ok()) << read_result.unwrap_err().message;
    EXPECT_EQ(read_result.unwrap(), test_data);

    // Test flush
    EXPECT_TRUE(storage.flush().is_ok());
}

TEST_F(BlockStorageTest, Concurrency) {
    BlockStorage storage;
    storage.mount(test_file.string(), config).unwrap();

    constexpr int num_threads = 4;
    constexpr int blocks_per_thread = 10;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&, i]() {
            std::vector<uint8_t> data(4096, static_cast<uint8_t>(i));
            for (int j = 0; j < blocks_per_thread; j++) {
                uint64_t block_id = i * blocks_per_thread + j;
                EXPECT_TRUE(storage.writeBlock(block_id, data).is_ok());
                auto read_result = storage.readBlock(block_id);
                ASSERT_TRUE(read_result.is_ok());
                EXPECT_EQ(read_result.unwrap(), data);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }
}

TEST_F(BlockStorageTest, EdgeCases) {
    // Test with minimum block size
    auto small_file = fs::temp_directory_path() / "small_blocks.bin";
    BlockStorage::create(small_file.string(), {512, 512*5}).unwrap();
    
    BlockStorage small_storage;
    small_storage.mount(small_file.string(), {512, 512*5}).unwrap();
    
    std::vector<uint8_t> small_data(512, 0xEE);
    EXPECT_TRUE(small_storage.writeBlock(2, small_data).is_ok());
    EXPECT_EQ(small_storage.readBlock(2).unwrap(), small_data);

    EXPECT_THROW(fs::remove(small_file), std::filesystem::filesystem_error);

    EXPECT_TRUE(small_storage.unmount().is_ok());
    EXPECT_NO_THROW(fs::remove(small_file));

    // Test with large block size
    auto large_file = fs::temp_directory_path() / "large_blocks.bin";
    BlockStorage::create(large_file.string(), {1024*1024, 1024*1024*2}).unwrap();
    
    BlockStorage large_storage;
    large_storage.mount(large_file.string(), {1024*1024, 1024*1024*2}).unwrap();
    
    std::vector<uint8_t> large_data(1024*1024, 0xFF);
    EXPECT_TRUE(large_storage.writeBlock(1, large_data).is_ok());
    EXPECT_EQ(large_storage.readBlock(1).unwrap(), large_data);
    EXPECT_TRUE(large_storage.unmount().is_ok());
    EXPECT_NO_THROW(fs::remove(large_file));
}

TEST_F(BlockStorageTest, FileValidation) {
    // 1. Test non-existent file
    {
        BlockStorage storage;
        auto result = storage.mount("nonexistent.bin", config);
        EXPECT_TRUE(result.is_err());
        EXPECT_EQ(result.unwrap_err().code, -4);
    }

    // 2. Test corrupted file (wrong size)
    {
        // Create a corrupted file
        fs::path corrupt_file = fs::temp_directory_path() / "corrupted.bin";
        {
            std::ofstream out(corrupt_file, std::ios::binary);
            out.write("CORRUPTED", 9);
        }

        BlockStorage storage;
        auto result = storage.mount(corrupt_file.string(), config);
        EXPECT_TRUE(result.is_err());
        EXPECT_EQ(result.unwrap_err().code, -5);

        fs::remove(corrupt_file);
    }

    // 3. Test directory instead of file
    {
        fs::path temp_dir = fs::temp_directory_path() / "temp_dir";
        fs::create_directory(temp_dir);

        BlockStorage storage;
        auto result = storage.mount(temp_dir.string(), config);
        EXPECT_TRUE(result.is_err());
        EXPECT_EQ(result.unwrap_err().code, -4);

        fs::remove(temp_dir);
    }
}

TEST_F(BlockStorageTest, ConfigValidation) {
    // 1. Test invalid block size (0)
    {
        BlockStorage storage;
        auto result = storage.mount(test_file.string(), {0, 4096*100});
        EXPECT_TRUE(result.is_err());
        EXPECT_EQ(result.unwrap_err().code, -6);
    }
}

TEST_F(BlockStorageTest, PerformanceBenchmark) {
    BlockStorage storage;
    storage.mount(test_file.string(), config).unwrap();

    constexpr int iterations = 10000;
    std::vector<uint8_t> data(4096, 0xAA);

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        storage.writeBlock(i % 100, data).unwrap();
    }
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start);

    std::cout << "Write performance: "
              << (iterations * 4) / duration.count() << " MB/s\n";
}