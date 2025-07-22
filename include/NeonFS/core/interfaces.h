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
} // namespace neonfs