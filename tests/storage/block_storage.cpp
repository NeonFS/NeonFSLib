#include <NeonFS/storage/block_storage.h>

neonfs::storage::BlockStorage::BlockStorage(std::string path) : path(std::move(path)) {
    if (path.empty()) {
        throw std::runtime_error("Storage path cannot be empty");
    }
}

neonfs::storage::BlockStorage::~BlockStorage() {
    if (is_mounted) unmount();
}

neonfs::Result<void> neonfs::storage::BlockStorage::mount(std::string _path, const BlockStorageConfig &_config) {
    std::lock_guard<std::mutex> lock(file_stream_mutex);
    if (is_mounted) {
        return Result<void>::err("Storage is already mounted", -1);
    }

    if (_path.empty()) {
        return Result<void>::err("Mount path cannot be empty", -2);
    }

    path = std::move(_path);
    filestream.open(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!filestream.is_open()) {
        return Result<void>::err("Failed to open storage file: " + path, -3);
    }

    is_mounted = true;
    block_size_ = _config.block_size;
    total_blocks_ = _config.total_size;
    return Result<void>::ok();
}

neonfs::Result<void> neonfs::storage::BlockStorage::unmount() {
    std::lock_guard<std::mutex> lock(file_stream_mutex);
    if (!is_mounted) {
        return Result<void>::err("Storage is not mounted", -1);
    }

    filestream.close();
    if (filestream.is_open()) {
        return Result<void>::err("Failed to close storage file", -2);
    }

    is_mounted = false;
    return Result<void>::ok();
}

bool neonfs::storage::BlockStorage::isMounted() const {
    return is_mounted;
}

neonfs::Result<void> neonfs::storage::BlockStorage::create(std::string path, BlockStorageConfig config) {
    size_t block_count = config.block_size / config.total_size;
    if (block_count < 1) return Result<void>::err("Invalid block count", -1);
    if (path.empty()) return Result<void>::err("Mount path cannot be empty", -2);
    std::lock_guard<std::mutex> lock(file_stream_mutex);
    std::ofstream c_filestream(path, std::ios::binary);
    if (!c_filestream.is_open()) return Result<void>::err("Failed to open storage file: " + path, -3);

    // Write empty blocks
    std::vector<uint8_t> empty_block(config.block_size, 0);
    for (size_t i = 0; i < block_count; i++) {
        c_filestream.write(reinterpret_cast<const char*>(empty_block.data()), empty_block.size());
    }
    c_filestream.flush();
    c_filestream.close();
    return Result<void>::ok();
}

neonfs::Result<std::vector<unsigned char> > neonfs::storage::BlockStorage::readBlock(uint64_t blockID) {
    std::lock_guard<std::mutex> lock(file_stream_mutex);
    if (!is_mounted) {
        return Result<std::vector<uint8_t>>::err("Storage is not mounted", -1);
    }

    if (blockID >= getBlockCount()) {
        return Result<std::vector<uint8_t>>::err("Invalid block ID", -2);
    }

    const uint64_t offset = blockID * block_size_;
    filestream.seekg(offset, std::ios::beg);
    if (!filestream.good()) {
        return Result<std::vector<uint8_t>>::err("Failed to seek to block position", -3);
    }

    std::vector<uint8_t> data(block_size_);
    filestream.read(reinterpret_cast<char*>(data.data()), block_size_);
    if (filestream.gcount() != static_cast<std::streamsize>(block_size_)) {
        return Result<std::vector<uint8_t>>::err("Incomplete block read", -4);
    }

    return Result<std::vector<uint8_t>>::ok(std::move(data));
}

neonfs::Result<void> neonfs::storage::BlockStorage::writeBlock(uint64_t blockID, std::vector<uint8_t> &data) {
    if (!is_mounted) {
        return Result<void>::err("Storage is not mounted", -1);
    }

    if (blockID >= getBlockCount()) {
        return Result<void>::err("Invalid block ID", -2);
    }

    if (data.size() > block_size_ ) {
        return Result<void>::err("Data size exceeds block size", -3);
    } else if (data.size() < block_size_) {
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
        std::lock_guard<std::mutex> lock(file_stream_mutex);
        filestream.seekp(offset, std::ios::beg);
        if (!filestream.good()) {
            return Result<void>::err("Failed to seek to block position", -4);
        }

        filestream.write(reinterpret_cast<const char*>(data.data()), block_size_);
        if (!filestream.good()) {
            return Result<void>::err("Failed to write block: possible disk full", -5);
        }
    }

    return Result<void>::ok();
}

neonfs::Result<void> neonfs::storage::BlockStorage::flush() {
    std::lock_guard<std::mutex> lock(file_stream_mutex);

    if (!is_mounted) {
        return Result<void>::err("Storage is not mounted");
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