# Usage of `BlockStorage`

This document provides a step-by-step guide on how to use the `neonfs::storage::BlockStorage` class to manage file-based block storage.

---

## The `BlockStorage` Lifecycle

The typical workflow for creating and using a storage file involves four main stages:
1.  **Creation**: Create a new, pre-sized storage file on disk. This is a one-time setup step.
2.  **Mounting**: Open the existing file to prepare it for I/O operations.
3.  **Read/Write**: Perform block-based I/O operations.
4.  **Unmounting**: Close the file handle.

---

### Step 1: Creating a New Storage File

Before you can use a storage volume, you must create its backing file. This is done using the static `BlockStorage::create()` method.

```cpp
#include <NeonFS/storage/block_storage.h>
#include <iostream>

void create_new_volume() {
    const std::string storage_path = "my_volume.dat";
    neonfs::BlockStorageConfig config;
    config.block_size = 4096; // 4 KB blocks
    config.total_size = 16 * 1024 * 1024; // 16 MB total size

    // Create the file and pre-allocate its size
    auto create_res = neonfs::storage::BlockStorage::create(storage_path, config);

    if (create_res.is_err()) {
        std::cerr << "Failed to create volume: " 
                  << create_res.unwrap_err().message << std::endl;
    } else {
        std::cout << "Volume created successfully at " << storage_path << std::endl;
    }
}
```

### Step 2: Mounting the Storage

To interact with an existing storage file, you must first instantiate `BlockStorage` and then `mount()` it.

```cpp
#include <NeonFS/storage/block_storage.h>

// Use the same config that the volume was created with
const std::string storage_path = "my_volume.dat";
neonfs::BlockStorageConfig config = {4096, 16 * 1024 * 1024};

// 1. Construct the manager object
neonfs::storage::BlockStorage storage();

// 2. Mount the volume to open the file
auto mount_res = storage.mount(storage_path, config);
if (mount_res.is_err()) {
    std::cerr << "Failed to mount volume: " 
              << mount_res.unwrap_err().message << std::endl;
    return; // Cannot proceed
}

// Check if mounted
if (storage.isMounted()) {
    std::cout << "Volume mounted. Block size: " << storage.getBlockSize()
              << ", Block count: " << storage.getBlockCount() << std::endl;
}
```

### Step 3: Writing and Reading Blocks

Once mounted, you can read from and write to any block using its zero-based ID.

```cpp
// (Continuing from the mounting example...)

// --- Writing a block ---
uint64_t block_id_to_write = 5;
std::vector<uint8_t> my_data = { 'H', 'e', 'l', 'l', 'o' };

// The writeBlock method automatically pads the data with zeros
// to match the full block size (4096 bytes in this case).
auto write_res = storage.writeBlock(block_id_to_write, my_data);
if (write_res.is_err()) {
    std::cerr << "Failed to write block " << block_id_to_write << std::endl;
} else {
    std::cout << "Wrote data to block " << block_id_to_write << std::endl;
}

// --- Reading a block ---
auto read_res = storage.readBlock(block_id_to_write);
if (read_res.is_err()) {
    std::cerr << "Failed to read block " << block_id_to_write << std::endl;
} else {
    std::vector<uint8_t> read_data = read_res.unwrap();
    std::cout << "Read " << read_data.size() << " bytes from block." << std::endl;
    // read_data[0] == 'H', read_data[4] == 'o', read_data[5] == 0 (padding)
}
```

### Step 4: Flushing and Unmounting

To ensure data is saved to disk, call `flush()`. When you are finished, `unmount()` the volume to release the file handle.

```cpp
// (Continuing from the previous example...)

// Force cached writes to disk
storage.flush();

// Unmount the volume
auto unmount_res = storage.unmount();
if (unmount_res.is_err()) {
    std::cerr << "Warning: Failed to unmount cleanly." << std::endl;
}

// Note: If the `storage` object goes out of scope, its destructor
// will automatically attempt to unmount it.
```

---

## Error Handling

All methods that perform I/O or state changes return a `neonfs::Result`. Always check if an operation failed before proceeding.

```cpp
auto res = storage.readBlock(99999); // Invalid block ID
if (res.is_err()) {
    // Handle the error gracefully
    neonfs::Error err = res.unwrap_err();
    std::cerr << "Error Code: " << err.code 
              << ", Message: " << err.message << std::endl;
}
```
