#pragma once
#include "result.hpp"
#include "types.h"
#include <vector>

namespace neonfs {
    class IEncryptionProvider {
    public:
        virtual ~IEncryptionProvider() = default;

        virtual Result<secure_vector<uint8_t>> encrypt(
            const secure_vector<uint8_t>& plain,
            secure_vector<uint8_t>& outIV,
            secure_vector<uint8_t>& outTag) = 0;

        virtual Result<secure_vector<uint8_t>> decrypt(
            const secure_vector<uint8_t>& cipher,
            const secure_vector<uint8_t>& iv,
            secure_vector<uint8_t>& tag) = 0;

        virtual size_t iv_size() const = 0;
        virtual size_t tag_size() const = 0;
    };

    class IStorageProvider {
    public:
        virtual ~IStorageProvider() = default;

        virtual Result<std::vector<uint8_t>> readBlock(uint64_t blockID) = 0;
        virtual Result<void> writeBlock(uint64_t blockID, std::vector<uint8_t>& data) = 0;
        [[nodiscard]] virtual uint64_t getBlockCount() const = 0;
        [[nodiscard]] virtual uint64_t getBlockSize() const = 0;
    };
} // namespace neonfs