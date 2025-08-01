# NeonFSLib Documentation

> **Note:** this documentation currently focuses on **internal modules** and implementation details.
> Public APIs and user-facing guids will be added once the external endpoints are finalized

---

## Documentation Structure

- `internal/` ─ Internal implementation docs, design notes and usage patterns for maintainers and contributors.
- `public/` ─ _(coming soon)_ Public API reference and high-level usage guides for developers using the library.

---

## Current Coverage

### Internal Modules

**Core**
- [internal/core/Result.md](internal/core/Result.md) — The `Result` class and error handling model.
- [internal/core/SecureAllocator.md](internal/core/SecureAllocator.md) — Secure memory allocation for sensitive data.

**Security**
- [internal/security/KeyManager.md](internal/security/KeyManager.md) — Generation, derivation, and verification of cryptographic keys.
- [internal/security/AESEncryptionProvider.md](internal/security/AESEncryptionProvider.md) — High-level AES-GCM encryption/decryption service.
- [internal/security/AESGCMCtx.md](internal/security/AESGCMCtx.md) — Low-level context for AES-GCM operations.
- [internal/security/AESGCMCtxPool.md](internal/security/AESGCMCtxPool.md) — A thread-safe pool for managing `AESGCMCtx` objects.

**Storage**
- [internal/storage/BlockStorage.md](internal/storage/BlockStorage.md) — File-based provider for fixed-size block I/O.

---

## Status

NeonFSLib is still in the **core infrastructure* phase*. Internal features are being built with performance and maintainability in mind.
Once the public-facing APIs are stable, a proper user guid will be added under `public/`.

Stay tuned!