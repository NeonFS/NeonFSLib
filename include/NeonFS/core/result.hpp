#pragma once
#include <optional>
#include <stdexcept>
#include <variant>
#include <string>
#include "types.h"

namespace neonfs {

    template<typename T>
    class Result {
    public:
        [[nodiscard]] static Result<T> ok(T value) {
            return Result<T>(std::move(value));
        }

        [[nodiscard]] static Result<T> err(const std::string &message, const int code = 0) {
            return Result<T>(Error{message, code});
        }

        [[nodiscard]] static Result<T> err(Error error) {
            return Result<T>(std::move(error));
        }

        [[nodiscard]] bool is_ok() const { return std::holds_alternative<T>(data_);  }
        [[nodiscard]] bool is_err() const { return std::holds_alternative<Error>(data_); }

        T& unwrap() {
            if (is_err()) {
                throw std::runtime_error("Attempted to unwrap error result: " + std::get<Error>(data_).message);
            }
            return std::get<T>(data_);
        }

        const T& unwrap() const {
            if (is_err()) {
                throw std::runtime_error("Attempted to unwrap error result: " + std::get<Error>(data_).message);
            }
            return std::get<T>(data_);
        }

        T unwrap_move() {
            if (is_err()) {
                throw std::runtime_error("Attempted to unwrap error result: " + std::get<Error>(data_).message);
            }
            return std::move(std::get<T>(data_));
        }

        [[nodiscard]] const Error& unwrap_err() const {
            if (is_ok()) {
                throw std::runtime_error("Attempted to unwrap_err on ok result");
            }
            return std::get<Error>(data_);
        }

        template<typename FOk, typename FErr>
        auto match(FOk&& ok_fn, FErr&& err_fn) {
            using OkResult = decltype(ok_fn(std::declval<T>()));
            using ErrResult = decltype(err_fn(std::declval<Error>()));
            static_assert(std::is_same_v<OkResult, ErrResult>,
                "Both handlers must return the same type");

            return std::visit(
                [&](auto&& arg) -> OkResult {  // Explicit return type
                    using Arg = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<Arg, T>) {
                        return std::forward<FOk>(ok_fn)(std::forward<decltype(arg)>(arg));
                    } else {
                        return std::forward<FErr>(err_fn)(std::forward<decltype(arg)>(arg));
                    }
                },
                data_
            );
        }

        T unwrap_or(T default_val) const {
            return is_ok() ? std::get<T>(data_) : default_val;
        }

        template<typename F>
        T unwrap_or_else(F&& f) {
            static_assert(
                std::is_invocable_v<F, Error>,
                "Function must accept Error and return T"
            );
            return is_ok() ? std::get<T>(data_) : f(std::get<Error>(data_));
        }

        std::optional<std::reference_wrapper<T>> try_unwrap() {
            if (is_err()) return std::nullopt;
            return std::ref(std::get<T>(data_));
        }

        [[nodiscard]] const Error& expect_err(const std::string& msg) const {
            if (is_ok()) {
                throw std::runtime_error(msg);
            }
            return std::get<Error>(data_);
        }

        template<typename F>
        auto and_then(F&& f) {
            if (is_err()) return std::invoke_result_t<F, T>::err(std::get<Error>(data_));
            return std::forward<F>(f)(std::get<T>(data_));
        }

        template<typename F>
        auto map(F&& f) {
            using ResultType = std::invoke_result_t<F, T>;

            if (is_err()) {
                return Result<ResultType>::err(std::get<Error>(data_));
            }

            if constexpr (std::is_void_v<ResultType>) {
                std::forward<F>(f)(std::get<T>(data_));
                return Result<void>::ok();
            } else {
                return Result<ResultType>::ok(std::forward<F>(f)(std::get<T>(data_)));
            }
        }

        template<typename F>
        Result<T> map_err(F&& f) {
            if (is_ok()) return *this;
            return Result<T>::err(f(std::get<Error>(data_))); // Apply `f` to the error
        }

        template<typename F>
        Result<T> or_else(F&& f) {
            if (is_ok()) return *this;
            return f(std::get<Error>(data_));  // Dereference the optional (we know it has value)
        }

        template<typename U>
        bool contains(const U& value) const {
            return is_ok() && std::get<T>(data_) == value;
        }

        // Transform to std::optional
        [[nodiscard]] std::optional<T> to_optional() const {
            if (is_err()) return std::nullopt;
            return std::get<T>(data_);
        }

    private:
        explicit Result(T value) : data_(std::move(value)) {}
        explicit Result(Error error) : data_(std::move(error)) {}

        std::variant<T, Error> data_;
    };

    // Specialization for Result<void>
    template<>
    class Result<void> {
    public:
        [[nodiscard]] static Result<void> ok() {
            return Result<void>(std::monostate{});
        }

        [[nodiscard]] static Result<void> err(const std::string& message, const int code = 0) {
            return Result<void>(Error{message, code});
        }

        [[nodiscard]] static Result<void> err(Error error) {
            return Result<void>(std::move(error));
        }

        [[nodiscard]] bool is_ok() const { return std::holds_alternative<std::monostate>(data_); }
        [[nodiscard]] bool is_err() const { return std::holds_alternative<Error>(data_); }

        void unwrap() const {
            if (is_err()) {
                throw std::runtime_error("Attempted to unwrap error result: " + std::get<Error>(data_).message);
            }
        }

        template<typename F>
        void unwrap_or_else(F&& f) {
            if (is_err()) f(std::get<Error>(data_));
        }

        [[nodiscard]] bool try_unwrap() const {
            return is_ok();
        }

        [[nodiscard]] const Error& unwrap_err() const {
            if (is_ok()) {
                throw std::runtime_error("Attempted to unwrap_err on ok result");
            }
            return std::get<Error>(data_);
        }

        [[nodiscard]] const Error& expect_err(const std::string& msg) const {
            if (is_ok()) {
                throw std::runtime_error(msg);
            }
            return std::get<Error>(data_);
        }

        template<typename FOk, typename FErr>
        auto match(FOk&& ok_fn, FErr&& err_fn) {
            using OkResult = decltype(ok_fn());
            using ErrResult = decltype(err_fn(std::declval<Error>()));
            static_assert(std::is_same_v<OkResult, ErrResult>,
                "Both handlers must return the same type");

            return std::visit(
                [&](auto&& arg) -> OkResult {  // Explicit return type
                    using Arg = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<Arg, std::monostate>) {
                        return std::forward<FOk>(ok_fn)();
                    } else {
                        return std::forward<FErr>(err_fn)(std::forward<decltype(arg)>(arg));
                    }
                },
                data_
            );
        }

        template<typename F>
        auto and_then(F&& f) {
            if (is_err()) return std::invoke_result_t<F>::err(std::get<Error>(data_));
            return std::forward<F>(f)();
        }

        template<typename F>
        auto map(F&& f) {
            using ResultType = std::invoke_result_t<F>;

            if (is_err()) {
                return Result<ResultType>::err(std::get<Error>(data_));
            }

            if constexpr (std::is_void_v<ResultType>) {
                std::forward<F>(f)();
                return Result<void>::ok();
            } else {
                return Result<ResultType>::ok(std::forward<F>(f)());
            }
        }

        template<typename F>
        Result<void> map_err(F&& f) {
            if (is_ok()) return *this;
            return Result<void>::err(f(std::get<Error>(data_)));
        }

        template<typename F>
        Result<void> or_else(F&& f) {
            if (is_ok()) return *this;
            return f(std::get<Error>(data_));
        }

    private:
        explicit Result(std::monostate) : data_(std::monostate{}) {}
        explicit Result(Error error) : data_(std::move(error)) {}

        std::variant<std::monostate, Error> data_;
    };
} // namespace neonfs
