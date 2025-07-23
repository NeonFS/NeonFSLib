# `AESGCMCtxPool` — Thread-Safe AES Context Pool

---
namespace:
- `neonfs::security`
---

## What is `AESGCMCtxPool`?

`AESGCMCtxPool` is a thread-safe object pool for managing and reusing `AESGCMCtx` instances. It is designed to significantly improve performance in applications that perform a high volume of concurrent encryption or decryption operations.

By reusing `EVP_CIPHER_CTX` objects, it avoids the overhead of repeated allocation and deallocation, which can be a bottleneck in performance-critical code.

## How It Works

The pool manages a collection of available `AESGCMCtx` objects. It is thread-safe and designed to be shared across multiple threads.

*   **Acquisition**: When a thread needs a context, it calls `acquire()`.
    *   If an idle context is in the pool, it is returned immediately.
    *   If the pool is empty but has not reached its maximum configured size, a new context is created.
    *   If the pool is empty and at maximum capacity, the thread will block until another thread releases a context.
*   **RAII-based Handle**: The `acquire()` method returns a `Handle`. This is a smart-pointer-like RAII object that guarantees the context is automatically returned to the pool when the handle goes out of scope.
*   **Release**: When a `Handle` is destroyed, its underlying `AESGCMCtx` is reset to a clean state and returned to the pool, making it available for another thread.

## The `Handle`

The `AESGCMCtxPool::Handle` is the primary mechanism for interacting with the pool.

*   **RAII Wrapper**: It ensures the context is always returned, even if exceptions occur.
*   **Pointer Semantics**: It overloads `operator->()` and `operator*()` so you can use it just like a pointer to an `AESGCMCtx`.
*   **Move-Only**: The handle has move semantics (`std::move`), ensuring clear ownership transfer.

## API Reference

### `AESGCMCtxPool(size_t maxSize)`
The constructor creates a pool with a hard limit on the number of concurrent `AESGCMCtx` objects it can manage.
- **`maxSize`**: The maximum number of `AESGCMCtx` objects the pool can contain.

### `std::shared_ptr<AESGCMCtxPool> create(size_t maxSize)`
A static factory function is the recommended way to create a pool, as it must be managed by a `std::shared_ptr`.

### `Handle acquire()`
Acquires a context from the pool, blocking if the pool is at maximum capacity and all contexts are in use.
- **Returns**: A `Handle` instance that provides access to an `AESGCMCtx`.

### `size_t availableCount()`
Returns the number of idle contexts currently available in the pool.

---

For practical examples and threading patterns, see the [AESGCMCtxPool Usage Guide](AESGCMCtxPoolUsage.md).