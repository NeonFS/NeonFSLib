#pragma once
#include <NeonFS/core/interfaces.h>
#include <fstream>
#include <mutex>
#include <filesystem>

namespace neonfs::storage {
    class BlockStorage final : public IStorageProvider {
        std::string path;
        bool is_mounted;
        std::fstream filestream;
        std::mutex file_stream_mutex;

        size_t block_size_ = 0;
        size_t total_blocks_ = 0;

        public:
        BlockStorage();
        ~BlockStorage() override;

        Result<void> mount(const StorageConfig &_config) override;
        Result<void> unmount() override;
        [[nodiscard]] bool isMounted() const override;
        static Result<void> create(const StorageConfig& config);

        Result<std::vector<uint8_t>> readBlock(uint64_t blockID) override;
        Result<void> writeBlock(uint64_t blockID, std::vector<uint8_t>& data) override;
        [[nodiscard]] uint64_t getBlockCount() const override;
        [[nodiscard]] uint64_t getBlockSize() const override;

        Result<void> resizeStorage(uint64_t newBlockCount) override;

        Result<void> flush() override;
    };
}// namespace neonfs::storage