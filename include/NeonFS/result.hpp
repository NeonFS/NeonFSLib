#pragma once
#include <optional>
#include <stdexcept>
#include <variant>
#include <string>

namespace neonfs {

    template<typename T>
    class Result {
    public:
        static Result<T> ok(T value) {
            return Result(std::move(value), std::string{});
        }

        static Result<T> err(std::string error) {
            return Result(T{}, std::move(error));
        }

        bool is_ok() const { return error_.empty(); }
        bool is_err() const { return !error_.empty(); }

        T& unwrap() {
            if (is_err()) {
                throw std::runtime_error("Attempted to unwrap error result: " + error_);
            }
            return value_;
        }

        const T& unwrap() const {
            if (is_err()) {
                throw std::runtime_error("Attempted to unwrap error result: " + error_);
            }
            return value_;
        }

        T unwrap_move() {
            if (is_err()) {
                throw std::runtime_error("Attempted to unwrap error result: " + error_);
            }
            return std::move(value_);
        }

        std::optional<std::reference_wrapper<T>> try_unwrap() {
            if (is_err()) return std::nullopt;
            return std::ref(value_);
        }

        const std::string& unwrap_err() const { return error_; }

        template<typename F>
        auto and_then(F&& f) {
            if (is_err()) return Result<std::invoke_result_t<F, T>>::err(error_);
            return f(value_);
        }

        template<typename F>
        auto map(F&& f) {
            if (is_err()) return Result<std::invoke_result_t<F, T>>::err(error_);
            return Result<std::invoke_result_t<F, T>>::ok(f(value_));
        }

        template<typename F>
        Result<T> or_else(F&& f) {
            if (is_ok()) return *this;
            return f(error_);
        }

    private:
        Result(T value, std::string error)
            : value_(std::move(value)), error_(std::move(error)) {}

        T value_;
        std::string error_;
    };

    // Specialization for Result<void>
    template<>
    class Result<void> {
    public:
        static Result<void> ok() {
            return Result<void>(std::string{});
        }

        static Result<void> err(std::string error) {
            return Result<void>(std::move(error));
        }

        bool is_ok() const { return error_.empty(); }
        bool is_err() const { return !error_.empty(); }

        void unwrap() const {
            if (is_err()) {
                throw std::runtime_error("Attempted to unwrap error result: " + error_);
            }
        }

        const std::string& unwrap_err() const { return error_; }

        template<typename F>
        auto and_then(F&& f) {
            if (is_err()) return Result<std::invoke_result_t<F>>::err(error_);
            return f();
        }

        template<typename F>
        Result<void> or_else(F&& f) {
            if (is_ok()) return *this;
            return f(error_);
        }

    private:
        explicit Result(std::string error)
            : error_(std::move(error)) {}

        std::string error_;
    };
} // namespace neonfs
