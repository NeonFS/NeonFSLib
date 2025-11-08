#include <NeonFS/storage/block_storage.h>

neonfs::storage::BlockStorage::BlockStorage() {
    is_mounted = false;
}

neonfs::storage::BlockStorage::~BlockStorage() {
    if (is_mounted) unmount();
}

neonfs::Result<void> neonfs::storage::BlockStorage::mount(const StorageConfig &_config) {
    const std::string _path = _config.resource_uri;
    std::lock_guard lock(file_stream_mutex);
    if (is_mounted) {
        return Result<void>::err("Storage is already mounted", -1);
    }

    if (_path.empty()) {
        return Result<void>::err("Mount path cannot be empty", -2);
    }

    std::error_code ec;
    if (!std::filesystem::exists(_path, ec) || !std::filesystem::is_regular_file(_path, ec)) {
        return Result<void>::err("Path is not a valid file", -4);
    }

    if (const auto file_size = std::filesystem::file_size(_path, ec); ec || file_size != _config.total_size) {
        return Result<void>::err("File size doesn't match configuration", -5);
    }

    if (_config.block_size == 0 || _config.total_size % _config.block_size != 0) {
        return Result<void>::err("Invalid block configuration", -6);
    }

    path = _path;
    filestream.open(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!filestream.is_open()) {
        return Result<void>::err("Failed to open storage file: " + path, -3);
    }

    is_mounted = true;
    block_size_ = _config.block_size;
    total_blocks_ = _config.total_size / _config.block_size;
    return Result<void>::ok();
}

neonfs::Result<void> neonfs::storage::BlockStorage::unmount() {
    std::lock_guard lock(file_stream_mutex);
    if (!is_mounted) {
        return Result<void>::err("Storage is not mounted", -1);
    }

    filestream.flush();
    filestream.close();

    // Reset state even if close failed
    is_mounted = false;
    if (filestream.is_open()) {
        return Result<void>::err("Failed to close storage file", -2);
    }

    return Result<void>::ok();
}

bool neonfs::storage::BlockStorage::isMounted() const {
    return is_mounted;
}

neonfs::Result<void> neonfs::storage::BlockStorage::create(const StorageConfig& config) {
    const std::string path = config.resource_uri;
    if (config.block_size == 0) return Result<void>::err("Block size cannot be zero", -4);
    if (config.total_size % config.block_size != 0) {
      return Result<void>::err("Total size must be a multiple of block size", -5);
    }

    const size_t block_count = config.total_size / config.block_size;
    if (block_count < 1) return Result<void>::err("Invalid block count", -1);
    if (path.empty()) return Result<void>::err("Mount path cannot be empty", -2);
    std::ofstream c_filestream(path, std::ios::binary);
    if (!c_filestream.is_open()) return Result<void>::err("Failed to open storage file: " + path, -3);

    // Write empty blocks
    const std::vector<uint8_t> empty_block(config.block_size, 0);
    for (size_t i = 0; i < block_count; i++) {
        c_filestream.write(reinterpret_cast<const char*>(empty_block.data()), static_cast<int64_t>(empty_block.size()));
    }
    c_filestream.flush();
    c_filestream.close();
    return Result<void>::ok();
}

neonfs::Result<std::vector<unsigned char> > neonfs::storage::BlockStorage::readBlock(const uint64_t blockID) {
    std::lock_guard lock(file_stream_mutex);
    if (!is_mounted) {
        return Result<std::vector<uint8_t>>::err("Storage is not mounted", -1);
    }

    if (blockID >= getBlockCount()) {
        return Result<std::vector<uint8_t>>::err("Invalid block ID", -2);
    }

    const uint64_t offset = blockID * block_size_;
    filestream.seekg(static_cast<int64_t>(offset), std::ios::beg);
    if (!filestream.good()) {
        return Result<std::vector<uint8_t>>::err("Failed to seek to block position", -3);
    }

    std::vector<uint8_t> data(block_size_);
    filestream.read(reinterpret_cast<char*>(data.data()), static_cast<int64_t>(block_size_));
    if (filestream.gcount() != static_cast<std::streamsize>(block_size_)) {
        return Result<std::vector<uint8_t>>::err("Incomplete block read", -4);
    }

    return Result<std::vector<uint8_t>>::ok(std::move(data));
}

neonfs::Result<void> neonfs::storage::BlockStorage::writeBlock(const uint64_t blockID, std::vector<uint8_t>& data) {
    if (!is_mounted) {
        return Result<void>::err("Storage is not mounted", -1);
    }

    if (blockID >= getBlockCount()) {
        return Result<void>::err("Invalid block ID", -2);
    }

    if (data.size() > block_size_ ) {
      return Result<void>::err("Data size exceeds block size", -3);
    }

    if (data.size() < block_size_) {
      std::vector<uint8_t> padding(block_size_ - data.size(), 0);
      data.insert(data.end(), padding.begin(), padding.end());
    }

    if (data.size() > block_size_) {
        return Result<void>::err("Data size does not match block size", -3);
    }

    if (data.size() < block_size_) {
        data.resize(block_size_, 0);
    }

    {
        const uint64_t offset = blockID * block_size_;
        std::lock_guard lock(file_stream_mutex);
        filestream.seekp(static_cast<int64_t>(offset), std::ios::beg);
        if (!filestream.good()) {
            return Result<void>::err("Failed to seek to block position", -4);
        }

        filestream.write(reinterpret_cast<const char*>(data.data()), static_cast<int64_t>(block_size_));
        if (!filestream.good()) {
            return Result<void>::err("Failed to write block: possible disk full", -5);
        }
    }

    return Result<void>::ok();
}

neonfs::Result<void> neonfs::storage::BlockStorage::flush() {
    std::lock_guard lock(file_stream_mutex);

    if (!is_mounted) {
        return Result<void>::err("Storage is not mounted", -1);
    }

    filestream.flush();
    if (!filestream) {
        return Result<void>::err("Flush failed");
    }

    return Result<void>::ok();
}

uint64_t neonfs::storage::BlockStorage::getBlockCount() const {
    return total_blocks_;
}

uint64_t neonfs::storage::BlockStorage::getBlockSize() const {
    return block_size_;
}

neonfs::Result<void> neonfs::storage::BlockStorage::resizeStorage(const uint64_t newBlockCount) {
    if (newBlockCount == total_blocks_) return Result<void>::ok();
    std::lock_guard lock(file_stream_mutex);
    if (!is_mounted) {
        return Result<void>::err("Storage is not mounted", -1);
    }

    // Flush any pending writings before resizing
    filestream.flush();
    if (!filestream) {
        return Result<void>::err("Flush failed before resize", -3);
    }

    if (newBlockCount > total_blocks_) {
        filestream.seekp(0, std::ios::end);
        const std::vector<uint8_t> empty_block(block_size_, 0);
        for (uint64_t i = total_blocks_; i < newBlockCount; i++) {
            filestream.write(reinterpret_cast<const char*>(empty_block.data()), static_cast<int64_t>(block_size_));
            if (!filestream) {
                return Result<void>::err("Failed to write new blocks during resize", -4);
            }
        }
    } else { // newBlockCount < total_blocks_
        // Shrink the file. This requires closing and reopening the file stream
        // as fstream doesn't support truncation directly.
        filestream.close();
        std::error_code ec;
        std::filesystem::resize_file(path, newBlockCount * block_size_, ec);
        if (ec) {
            // Attempt to reopen the file even if resize failed to restore the state
            filestream.open(path, std::ios::binary | std::ios::in | std::ios::out);
            return Result<void>::err("Failed to resize storage file: " + ec.message(), -2);
        }
        filestream.open(path, std::ios::binary | std::ios::in | std::ios::out);
        if (!filestream.is_open()) {
            is_mounted = false; // Critical error, mark as unmounted
            return Result<void>::err("Failed to reopen storage file after resize", -5);
        }
    }

    total_blocks_ = newBlockCount;
    return Result<void>::ok();
}