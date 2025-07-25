#pragma once
#include <NeonFS/core/interfaces.h>
#include <fstream>
#include <mutex>

namespace neonfs::storage {
    struct BlockStorageConfig {
        size_t block_size;
        size_t total_size;
        bool encrypted;
    };

    class BlockStorage final : public IStorageProvider {
        std::string path;
        bool is_mounted = false;
        std::fstream filestream;
        std::mutex file_stream_mutex;

        size_t block_size_;

        public:
        BlockStorage(std::string path);

        Result<void> mount(std::string _path);
        Result<void> unmount();
        bool isMounted() const;
        Result<void> create(std::string path, BlockStorageConfig config);

        Result<std::vector<uint8_t>> readBlock(uint64_t blockID) override;
        Result<void> writeBlock(uint64_t blockID, std::vector<uint8_t>& data) override;
        uint64_t getBlockCount() const override;
        uint64_t getBlockSize() const override;

        Result<void> flush();
    };
}// namespace neonfs::storage