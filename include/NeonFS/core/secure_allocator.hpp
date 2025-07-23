#pragma once
#include <openssl/crypto.h>
#include <memory>
#include <limits>
#include <new>
#include <type_traits>
#include <iostream>

namespace neonfs {
	inline void initialize_secure_heap(const size_t size = 64 * 1024 * 1024, const size_t min_allocation = 64) {
		if (!CRYPTO_secure_malloc_initialized()) {
			if (!CRYPTO_secure_malloc_init(size, min_allocation)) {
				throw std::runtime_error("Failed to initialize OpenSSL secure heap");
			}
		}
	}

	inline void cleanup_secure_heap() {
		if (CRYPTO_secure_malloc_initialized()) {
			if (!CRYPTO_secure_malloc_done()) {
				throw std::runtime_error("Failed to shut down OpenSSL secure heap — possibly still in use");
			}
		}
	}


	template<typename T>
	class secure_allocator
	{
		static_assert(std::is_trivially_destructible_v<T>,
			"secure_allocator requires trivially destructible types to guarantee secure wiping.");
	public:
		using value_type = T;
		using pointer = T*;
		using const_pointer = const T*;
		using reference = T&;
		using const_reference = const T&;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;

		secure_allocator() noexcept = default;
		secure_allocator(const secure_allocator&) noexcept = default;
		secure_allocator& operator=(const secure_allocator&) noexcept = default;

		template<typename U>
		explicit secure_allocator(const secure_allocator<U>&) noexcept {}

		[[nodiscard]] T* allocate(const std::size_t n)
		{
			if (n == 0) return nullptr;

			if (n > max_size()) throw std::bad_alloc();

			if (!CRYPTO_secure_malloc_initialized()) throw std::runtime_error("OpenSSL secure heap not initialized");

			const std::size_t total_bytes = n * sizeof(T);

			void* p = OPENSSL_secure_malloc(total_bytes);
			if (!p) {
				std::cout << "Failed to allocate " << total_bytes << " bytes" << std::endl;
				throw std::bad_alloc();
			}

			return static_cast<T*>(p);
		}

		void deallocate(T* p, const std::size_t n ) noexcept
		{
			if (!p) return;
			const std::size_t total_bytes = n * sizeof(T);
			OPENSSL_secure_clear_free(p, total_bytes); // Wipe + free
		}

		[[nodiscard]] std::size_t max_size() noexcept
		{
			return std::numeric_limits<std::size_t>::max() / sizeof(T);
		}

		template<typename U>
		struct rebind
		{
			using other = secure_allocator<U>;
		};

		bool operator==(const secure_allocator&) const noexcept { return true; }
		bool operator!=(const secure_allocator&) const noexcept { return false; }
	};
} // namespace neonfs