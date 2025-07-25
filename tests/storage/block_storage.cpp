#include <NeonFS/storage/block_storage.h>

neonfs::storage::BlockStorage::BlockStorage(std::string path) : path(std::move(path)) {
    if (path.empty()) {
        throw std::runtime_error("Storage path cannot be empty");
    }
}

neonfs::Result<void> neonfs::storage::BlockStorage::mount(std::string _path) {
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

    if (data.size() != block_size_) {
        return Result<void>::err("Data size does not match block size", -3);
    }

    const uint64_t offset = blockID * block_size_;
    {
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
    if (!is_mounted) {
        return Result<void>::err("Storage is not mounted", -1);
    }
    std::lock_guard<std::mutex> lock(file_stream_mutex);
    filestream.flush();
    return Result<void>::ok();
}
