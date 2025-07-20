#pragma once
#include <optional>
#include <stdexcept>
#include <variant>
#include <string>

namespace neonfs {

    struct Error {
        std::string message;
        int code = 0;
    };

    template<typename T>
    class Result {
    public:
        static Result<T> ok(T value) {
            return Result(std::move(value), std::nullopt);
        }

        static Result<T> err(const std::string& message, const int code = 0) {
            return Result<T>(T{}, std::make_optional(Error{message, code}));
        }

        static Result<T> err(Error error) {
            return Result(T{}, std::make_optional(std::move(error)));
        }

        bool is_ok() const { return !error_.has_value(); }
        bool is_err() const { return error_.has_value(); }

        T& unwrap() {
            if (is_err()) {
                throw std::runtime_error("Attempted to unwrap error result: " + error_->message);
            }
            return value_;
        }

        const T& unwrap() const {
            if (is_err()) {
                throw std::runtime_error("Attempted to unwrap error result: " + error_->message);
            }
            return value_;
        }

        T unwrap_move() {
            if (is_err()) {
                throw std::runtime_error("Attempted to unwrap error result: " + error_->message);
            }
            return std::move(value_);
        }

        std::optional<std::reference_wrapper<T>> try_unwrap() {
            if (is_err()) return std::nullopt;
            return std::ref(value_);
        }

        [[nodiscard]] const std::optional<Error>& unwrap_err() const { return error_; }

        template<typename F>
        auto and_then(F&& f) {
            if (is_err()) return Result<std::invoke_result_t<F, T>>::err(error_);
            return std::forward<F>(f)(value_);
        }

        template<typename F>
        auto map(F&& f) {
            if (is_err()) return Result<std::invoke_result_t<F, T>>::err(error_);
            return Result<std::invoke_result_t<F, T> >::ok(std::forward<F>(f)(value_));
        }

        template<typename F>
        Result<T> or_else(F&& f) {
            if (is_ok()) return *this;
            return f(error_);
        }

        template<typename U>
        bool contains(const U& value) const {
            return is_ok() && value_ == value;
        }

        // Transform to std::optional
        std::optional<T> to_optional() const {
            if (is_err()) return std::nullopt;
            return value_;
        }

    private:
        Result(T value, std::optional<Error> error)
            : value_(std::move(value)), error_(std::move(error)) {}

        T value_;
        std::optional<Error> error_;
    };

    // Specialization for Result<void>
    template<>
    class Result<void> {
    public:
        static Result<void> ok() {
            return Result<void>(std::nullopt);
        }

        static Result<void> err(const std::string& message, const int code = 0) {
            return Result<void>(std::make_optional(Error{message, code}));
        }

        static Result<void> err(Error error) {
            return Result<void>(std::make_optional(std::move(error)));
        }

        bool is_ok() const { return !error_.has_value(); }
        bool is_err() const { return error_.has_value(); }

        void unwrap() const {
            if (is_err()) {
                throw std::runtime_error("Attempted to unwrap error result: " + error_->message);
            }
        }

        [[nodiscard]] bool try_unwrap() const {
            return !is_err();
        }

        [[nodiscard]] const std::optional<Error>& unwrap_err() const { return error_; }

        template<typename F>
        auto and_then(F&& f) {
            if (is_err()) return Result<std::invoke_result_t<F>>::err(error_);
            return std::forward<F>(f)();
        }

        template<typename F>
        auto map(F&& f) {
            if (is_err()) return Result<std::invoke_result_t<F>>::err(error_);
            return Result<std::invoke_result_t<F>>::ok(f());
        }

        template<typename F>
        Result<void> or_else(F&& f) {
            if (is_ok()) return *this;
            return f(error_);
        }

    private:
        explicit Result(std::optional<Error> error)
            : error_(std::move(error)) {}

        std::optional<Error> error_;
    };
} // namespace neonfs
