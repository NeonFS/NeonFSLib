# `BlockStorage` — File-Based Block I/O Provider

---
namespace:
- `neonfs::storage`
---

> **NOTE:** `BlockStorage` does not verify consistency or corruption.

## Overview

`BlockStorage` is a concrete implementation of the `IStorageProvider` interface. It provides a low-level abstraction for managing a file on disk as a collection of fixed-size blocks. This class is responsible for creating, mounting, reading, and writing data in discrete, block-aligned chunks.

It is designed for thread-safe, direct block manipulation, making it a foundational layer for building higher-level storage systems like a file system or a database.

### Key Features
*   **File-Backed:** Manages a single file on the local filesystem.
*   **Fixed-Size Blocks:** Divides the file into a predefined number of blocks, each with a fixed size.
*   **Thread-Safe:** All file I/O operations (`readBlock`, `writeBlock`, `flush`) are protected by a mutex, allowing safe use across multiple threads.
*   **Interface Compliant:** Implements the `IStorageProvider` interface, making it interchangeable with other storage backends.

---

## Configuration

Operations rely on the `BlockStorageConfig` struct to define the geometry of the storage file.

```cpp
struct BlockStorageConfig {
    size_t block_size; // Size of each block in bytes (e.g., 4096)
    size_t total_size; // Total size of the storage file in bytes
};
```

The `total_size` must be an exact multiple of the `block_size`.

---

## API Reference

### Constructor & Destructor

**`BlockStorage()`**
Constructs the storage manager. It does *not* open or create the file.

**`~BlockStorage()`**
The destructor automatically calls `unmount()` if the storage is currently mounted, ensuring the file handle is properly released.

### Lifecycle Management

**`static Result<void> create(std::string path, BlockStorageConfig config)`**
A static utility method that creates a new, empty storage file at the given `path`. The file is pre-allocated to `config.total_size` and zero-filled. This must be called before a new storage file can be mounted.

**`Result<void> mount(std::string path, const BlockStorageConfig& config)`**
Opens the storage file at the specified `path` for reading and writing. The file must already exist. This method prepares the `BlockStorage` instance for I/O operations.

**`Result<void> unmount()`**
Closes the file handle, effectively unmounting the storage. No further I/O can be performed until `mount()` is called again.

**`bool isMounted() const`**
Returns `true` if the storage file is currently open and mounted, `false` otherwise.

### I/O Operations

**`Result<std::vector<uint8_t>> readBlock(uint64_t blockID)`**
Reads the full contents of the block specified by `blockID`.
*   **Returns:** A [Result](../core/Result.md) containing the data as a `std::vector<uint8_t>` on success. The vector's size will equal the block size. Returns an error if the block ID is out of bounds or a read failure occurs.

**`Result<void> writeBlock(uint64_t blockID, std::vector<uint8_t>& data)`**
Writes the contents of the `data` vector to the block specified by `blockID`.
*   If `data` is smaller than the block size, it will be padded with zeros to fill the entire block.
*   If `data` is larger than the block size, the operation will fail.

**`Result<void> flush()`**
Flushes the underlying file stream's buffer, forcing any cached writes to be persisted to the disk.

### Getters

**`uint64_t getBlockCount() const`**
Returns the total number of blocks in the storage volume.

**`uint64_t getBlockSize() const`**
Returns the size of a single block in bytes.

---

## Thread Safety

All methods that interact with the underlying `std::fstream` (`mount`, `unmount`, `readBlock`, `writeBlock`, `flush`) are internally synchronized with a `std::mutex`. This guarantees that file operations are atomic and prevents data corruption when a single `BlockStorage` instance is accessed by multiple threads.

---

For practical, complete code examples, see the [BlockStorage Usage Guide](BlockStorageUsage.md).