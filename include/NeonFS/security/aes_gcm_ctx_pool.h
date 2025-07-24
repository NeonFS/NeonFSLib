#pragma once
#include <NeonFS/security/aes_gcm_ctx.h>
#include <memory>
#include <mutex>
#include <stack>

namespace neonfs::security {
    class AESGCMCtxPool : public std::enable_shared_from_this<AESGCMCtxPool> {
    public:
        class Handle {
            std::shared_ptr<AESGCMCtxPool> pool;
            std::unique_ptr<AESGCMCtx> ctx;
        public:
            Handle(std::shared_ptr<AESGCMCtxPool> p, std::unique_ptr<AESGCMCtx> c);
            ~Handle();

            AESGCMCtx* operator->() const;
            AESGCMCtx& operator*() const;

            void reset();


            Handle(const Handle&) = delete;
            Handle& operator=(const Handle&) = delete;
            Handle(Handle&&) = default;
            Handle& operator=(Handle&& other) noexcept;
        };

        AESGCMCtxPool(size_t maxSize);
        static std::shared_ptr<AESGCMCtxPool> create(size_t maxSize);
        Handle acquire();
        size_t availableCount();
    private:
        void release(std::unique_ptr<AESGCMCtx> ctx);

        std::mutex mutex;
        std::condition_variable condVar;
        std::stack<std::unique_ptr<AESGCMCtx>> pool;
        size_t currentSize = 0;
        const size_t maxPoolSize;
    };
} // namespace neon::security