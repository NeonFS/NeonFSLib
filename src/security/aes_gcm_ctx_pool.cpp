#include <NeonFS/security/aes_gcm_ctx_pool.h>

neonfs::security::AESGCMCtxPool::Handle::Handle(std::shared_ptr<AESGCMCtxPool> p, std::unique_ptr<AESGCMCtx> c): pool(std::move(p)), ctx(std::move(c)) {}

neonfs::security::AESGCMCtxPool::Handle::~Handle() {
    reset();
}

void neonfs::security::AESGCMCtxPool::Handle::reset() {
    if (pool && ctx) {
        pool->release(std::move(ctx));
        pool.reset(); // Clear the pool reference
    }
    ctx.reset(); // Double-clear for safety
}

neonfs::security::AESGCMCtxPool::Handle &neonfs::security::AESGCMCtxPool::Handle::operator=(Handle &&other) noexcept {
    if (this != &other) {
        reset(); // Release current context first
        pool = std::move(other.pool);
        ctx = std::move(other.ctx);
    }
    return *this;
}

neonfs::security::AESGCMCtx* neonfs::security::AESGCMCtxPool::Handle::operator->() const {
    return ctx.get();
}

neonfs::security::AESGCMCtx& neonfs::security::AESGCMCtxPool::Handle::operator*() const {
    return *ctx;
}

neonfs::security::AESGCMCtxPool::AESGCMCtxPool(size_t maxSize) : maxPoolSize(maxSize) {}

neonfs::security::AESGCMCtxPool::Handle neonfs::security::AESGCMCtxPool::acquire() {
    std::unique_lock<std::mutex> lock(mutex);
    if (!pool.empty()) {
        std::unique_ptr<AESGCMCtx> ctx = std::move(pool.top());
        pool.pop();
        return Handle(shared_from_this(), std::move(ctx));
    }
    else if (currentSize < maxPoolSize) {
        ++currentSize;
        lock.unlock();
        auto newCtx = std::make_unique<AESGCMCtx>();
        return Handle(shared_from_this(), std::move(newCtx));
    }
    else {
        condVar.wait(lock, [this] { return !pool.empty(); });
        std::unique_ptr<AESGCMCtx> ctx = std::move(pool.top());
        pool.pop();
        return Handle(shared_from_this(), std::move(ctx));
    }
}

void neonfs::security::AESGCMCtxPool::release(std::unique_ptr<AESGCMCtx> ctx) {
    std::lock_guard<std::mutex> lock(mutex);
    ctx->reset();  // reset ctx state before returning to pool
    pool.push(std::move(ctx));
    condVar.notify_one();
}

size_t neonfs::security::AESGCMCtxPool::availableCount() {
    std::lock_guard<std::mutex> lock(mutex);
    return pool.size();
}
