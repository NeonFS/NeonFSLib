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

    class IMetadataProvider {
    public:
        virtual ~IMetadataProvider() = default;

        /**
         * @brief Initialize the metadata provider (e.g., open DB, load cache)
         */
        virtual void initialize() = 0;

        /**
         * @brief Shut down the provider, flushing any pending writes.
         */
        virtual void shutdown() = 0;

        /**
         * @brief Store or update metadata entry.
         * @param meta The metadata record to upsert.
         */
        virtual void upsertMetadata(const Metadata &meta) = 0;


        /**
         * @brief Retrieve metadata by its unique file ID.
         * @param fileId The ID of the metadata record.
         * @return Metadata record if found, otherwise throws or returns empty optional.
         */
        virtual Metadata getMetadata(uint64_t fileId) = 0;

        /**
         * @brief Delete a metadata record.
         * @param fileId The ID of the record to remove.
         */
        virtual void deleteMetadata(uint64_t fileId) = 0;

        /**
         * @brief List all metadata file IDs currently stored.
         * @return Vector of file IDs.
         */
        virtual std::vector<uint64_t> listMetadataIds() = 0;

        /**
         * @brief Verify integrity of a metadata entry (e.g., checksum, block list).
         * @param meta The metadata record to verify.
         * @return true if valid, false otherwise.
         */
        virtual bool verifyMetadata(const Metadata &meta) = 0;

        /**
         * @brief Batch fetch metadata records for given file IDs.
         * @param ids Vector of file IDs to retrieve.
         * @return Vector of Metadata records.
         */
        virtual std::vector<Metadata> batchGetMetadata(const std::vector<uint64_t> &ids) = 0;

        /**
         * @brief Get all children (files or directories) of a directory.
         * @param parentId ID of the parent directory.
         * @return Vector of Metadata entries representing the children.
         */
        virtual std::vector<Metadata> getChildren(uint64_t parentId) = 0;

        /**
         * @brief Check if a directory is empty (has no children).
         * @param directoryId ID of the directory to check.
         * @return true if empty, false otherwise.
         */
        virtual bool isDirectoryEmpty(uint64_t directoryId) = 0;

        /**
         * @brief Move a file or directory to a new parent directory.
         * @param fileId The ID of the file or directory to move.
         * @param newParentId The ID of the new parent directory.
         */
        virtual void move(uint64_t fileId, uint64_t newParentId) = 0;

        /**
         * @brief Create a new directory.
         * @param name Name of the directory.
         * @param parentId ID of the parent directory.
         * @param permissions Optional permission bitmask.
         * @return ID of the newly created directory.
         */
        virtual uint64_t createDirectory(const std::string &name, uint64_t parentId, uint32_t permissions) = 0;

        /**
         * @brief Create a new empty file.
         * @param name Name of the file.
         * @param parentId ID of the parent directory.
         * @param permissions Optional permission bitmask.
         * @return ID of the newly created file.
         */
        virtual uint64_t createFile(const std::string &name, uint64_t parentId, uint32_t permissions) = 0;


        /**
         * @brief Rename a file or directory.
         * @param fileId ID of the file or directory to rename.
         * @param newName New name for the file or directory.
         */
        virtual void rename(uint64_t fileId, const std::string &newName) = 0;
    };
} // namespace neonfs