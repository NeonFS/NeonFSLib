#pragma once
#include "result.hpp"
#include "types.h"
#include <vector>

namespace neonfs {
    class IEncryptionProvider {
    public:
        virtual ~IEncryptionProvider() = default;

        virtual Result<std::vector<uint8_t>> encrypt(
            const secure_vector<uint8_t>& plain,
            std::vector<uint8_t>& outIV,
            std::vector<uint8_t>& outTag) = 0;

        virtual Result<secure_vector<uint8_t>> decrypt(
            const std::vector<uint8_t>& cipher,
            const std::vector<uint8_t>& iv,
            std::vector<uint8_t>& tag) = 0;

        virtual size_t iv_size() const = 0;
        virtual size_t tag_size() const = 0;
    };

    class IStorageProvider {
    public:
        virtual ~IStorageProvider() = default;

        // Lifecycle Management //

        // Initializes the storage provider with the specified configuration.
        virtual Result<void> mount(const StorageConfig &config) = 0;

        // Gracefully shuts down the storage provider and releases resources.
        virtual  Result<void> unmount() = 0;

        // Returns true if the storage provider is properly initialized.
        virtual bool isMounted() const = 0;

        // Block Operations //

        // Reads the full contents of a block by its identifier.
        virtual Result<std::vector<uint8_t> > readBlock(uint64_t blockID) = 0;

        // Writes data to a block. Data is padded or truncated to block size.
        virtual Result<void> writeBlock(uint64_t blockID, std::vector<uint8_t>& data) = 0;

        [[nodiscard]] virtual uint64_t getBlockCount() const = 0;

        // Returns the size of each block in bytes.
        [[nodiscard]] virtual uint64_t getBlockSize() const = 0;

        // Returns comprehensive storage statistics and usage information.
        // virtual Result<StorageStats> getStorageStats() = 0;

        // Capacity Management //

        // Resizes the storage to accommodate the specified number of blocks.
        virtual Result<void> resizeStorage(uint64_t newBlockCount) = 0;

        virtual Result<void> flush() = 0;
    };

    class IMetadataProvider {
    public:
        virtual ~IMetadataProvider() = default;

        virtual Result<void> initialize() = 0;

        virtual Result<void> shutdown() = 0;

    };
} // namespace neonfs
