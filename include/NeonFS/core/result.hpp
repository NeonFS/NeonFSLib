#pragma once
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <functional>
#include <utility> // For std::in_place_index

#include "types.h"

namespace neonfs {

    template<typename T>
    class Result;
    template<>
    class Result<void>;

    template<typename>
    struct is_result : std::false_type {};
    template<typename T>
    struct is_result<Result<T>> : std::true_type {};
    template<typename T>
    inline constexpr bool is_result_v = is_result<T>::value;

    template<typename F, typename T>
    concept returns_result_for = std::invocable<F, T> && is_result_v<std::invoke_result_t<F, T>>;

    template<typename F>
    concept returns_result_for_void = std::invocable<F> && is_result_v<std::invoke_result_t<F>>;

    // Specialization for Result<void>
    template<>
    class Result<void> {
    public:
        using value_type = void;
        using error_type = Error;

        [[nodiscard]] static Result<void> ok() {
            return Result<void>(std::in_place_index<0>);
        }

        [[nodiscard]] static Result<void> err(const std::string& message, const int code = 0) {
            return Result<void>(std::in_place_index<1>, Error{message, code});
        }

        [[nodiscard]] static Result<void> err(Error error) {
            return Result<void>(std::in_place_index<1>, std::move(error));
        }

        template<typename... EArgs>
            requires std::constructible_from<Error, EArgs...>
        [[nodiscard]] static Result<void> err_in_place(EArgs&&... args) {
            return Result<void>(std::in_place_index<1>, std::forward<EArgs>(args)...);
        }

        [[nodiscard]] bool is_ok() const { return std::holds_alternative<std::monostate>(data_); }
        [[nodiscard]] bool is_err() const { return std::holds_alternative<Error>(data_); }
        explicit operator bool() const noexcept { return is_ok(); }

        void expect(const std::string& msg) const {
            std::visit(
                [&]<typename T0>(const T0& arg) -> void {
                    using Args = std::decay_t<T0>;

                    if constexpr (std::is_same_v<Args, Error>) {
                        throw std::runtime_error(msg + ": " + arg.message);
                    }
                },
                data_
            );
        }

        void unwrap() {
            return std::visit(
                [&]<typename T0>(T0&& arg) -> void {
                    using Arg = std::decay_t<T0>;

                    if constexpr (std::is_same_v<Arg, Error>) {
                        throw std::runtime_error("Attempted to unwrap error result: " + arg.message);
                    }
                },
            data_);
        }

        void unwrap_or_else(std::function<void(Error&)> f) & {
            std::visit(
                [&]<typename T0>(T0& arg) {
                    if constexpr (std::is_same_v<std::decay_t<T0>, Error>) {
                        f(arg);
                    }
                },
           data_);
        }

        void unwrap_or_else(std::function<void(const Error&)> f) const & {
            std::visit(
                [&]<typename T0>(const T0& arg) {
                    if constexpr (std::is_same_v<std::decay_t<T0>, Error>) {
                        f(arg);
                    }
                },
           data_);
        }

        void unwrap_or_else(std::function<void(Error&&)> f) && {
            std::visit(
                [&]<typename T0>(T0&& arg) {
                    if constexpr (std::is_same_v<std::decay_t<T0>, Error>) {
                        f(std::move(arg));
                    }
                },
           data_);
        }

        // Removed try_unwrap(), which was redundant with to_optional()

        [[nodiscard]] Error& unwrap_err() & {
            return std::get<Error>(data_);
        }
        [[nodiscard]] const Error& unwrap_err() const & {
            return std::get<Error>(data_);
        }
        [[nodiscard]] Error&& unwrap_err() && {
            return std::move(std::get<Error>(data_));
        }
        [[nodiscard]] const Error&& unwrap_err() const && {
            return std::move(std::get<Error>(data_));
        }

        [[nodiscard]] Error& expect_err(const std::string& msg) & {
            return std::visit(
                  [&]<typename T0>(T0&& arg) -> Error& {
                  if constexpr (std::is_same_v<std::decay_t<T0>, Error>) {
                      return arg;
                  } else {
                      throw std::runtime_error(msg);
                  }
            },
            data_);
        }
        [[nodiscard]] const Error& expect_err(const std::string& msg) const & {
            return std::visit(
                  [&]<typename T0>(T0&& arg) -> const Error& {
                  if constexpr (std::is_same_v<std::decay_t<T0>, Error>) {
                      return arg;
                  } else {
                      throw std::runtime_error(msg);
                  }
            },
            data_);
        }
        [[nodiscard]] Error&& expect_err(const std::string& msg) && {
            return std::visit(
                  [&]<typename T0>(T0&& arg) -> Error&& {
                  if constexpr (std::is_same_v<std::decay_t<T0>, Error>) {
                      return std::move(arg);
                  } else {
                      throw std::runtime_error(msg);
                  }
            },
            data_);
        }
        [[nodiscard]] const Error&& expect_err(const std::string& msg) const && {
            return std::visit(
                  [&]<typename T0>(T0&& arg) -> const Error&& {
                  if constexpr (std::is_same_v<std::decay_t<T0>, Error>) {
                      return std::move(arg);
                  } else {
                      throw std::runtime_error(msg);
                  }
            },
            data_);
        }

        template<typename FOk, typename FErr>
          requires std::invocable<FOk> &&
              std::invocable<FErr, Error&> && // Corrected concept
              std::is_same_v<std::invoke_result_t<FOk>, std::invoke_result_t<FErr, Error&>>
        auto match(FOk&& ok_fn, FErr&& err_fn) & {
            using ResultType = std::invoke_result_t<FOk>;

            return std::visit(
                [&]<typename T0>(T0&& arg) -> ResultType {
                    if constexpr (std::is_same_v<std::decay_t<T0>, std::monostate>) {
                        return std::invoke(std::forward<FOk>(ok_fn));
                    } else {
                        return std::invoke(std::forward<FErr>(err_fn), arg);
                    }
                },
                data_
            );
        }
        template<typename FOk, typename FErr>
          requires std::invocable<FOk> &&
              std::invocable<FErr, Error&> &&
              std::is_same_v<std::invoke_result_t<FOk>, std::invoke_result_t<FErr, Error&>>
        auto match(FOk&& ok_fn, FErr&& err_fn) const & {
            using ResultType = std::invoke_result_t<FOk>;

            return std::visit(
                [&]<typename T0>(T0&& arg) -> ResultType {
                    if constexpr (std::is_same_v<std::decay_t<T0>, std::monostate>) {
                        return std::invoke(std::forward<FOk>(ok_fn));
                    } else {
                        return std::invoke(std::forward<FErr>(err_fn), arg);
                    }
                },
                data_
            );
        }
        template<typename FOk, typename FErr>
          requires std::invocable<FOk> &&
              std::invocable<FErr, Error&> &&
              std::is_same_v<std::invoke_result_t<FOk>, std::invoke_result_t<FErr, Error&>>
        auto match(FOk&& ok_fn, FErr&& err_fn) && {
            using ResultType = std::invoke_result_t<FOk>;

            return std::visit(
                [&]<typename T0>(T0&& arg) -> ResultType {
                    if constexpr (std::is_same_v<std::decay_t<T0>, std::monostate>) {
                        return std::invoke(std::forward<FOk>(ok_fn));
                    } else {
                        return std::invoke(std::forward<FErr>(err_fn), std::move(arg));
                    }
                },
                data_
            );
        }
        template<typename FOk, typename FErr>
          requires std::invocable<FOk> &&
              std::invocable<FErr, Error&> &&
              std::is_same_v<std::invoke_result_t<FOk>, std::invoke_result_t<FErr, Error&>>
        auto match(FOk&& ok_fn, FErr&& err_fn) const && {
            using ResultType = std::invoke_result_t<FOk>;

            return std::visit(
                [&]<typename T0>(T0&& arg) -> ResultType {
                    if constexpr (std::is_same_v<std::decay_t<T0>, std::monostate>) {
                        return std::invoke(std::forward<FOk>(ok_fn));
                    } else {
                        return std::invoke(std::forward<FErr>(err_fn), std::move(arg));
                    }
                },
                data_
            );
        }

        template<typename F> requires std::invocable<F>
        auto map(F&& f) & {
            return map_impl(*this, std::forward<F>(f));
        }
        template<typename F> requires std::invocable<F>
        auto map(F&& f) const & {
            return map_impl(*this, std::forward<F>(f));
        }
        template<typename F> requires std::invocable<F>
        auto map(F&& f) && {
            return map_impl(std::move(*this), std::forward<F>(f));
        }
        template<typename F> requires std::invocable<F>
        auto map(F&& f) const && {
            return map_impl(std::move(*this), std::forward<F>(f));
        }

        template<typename F> requires returns_result_for_void<F>
        auto and_then(F&& f) & {
            return and_then_impl(*this, std::forward<F>(f));
        }
        template<typename F> requires returns_result_for_void<F>
        auto and_then(F&& f) const & {
            return and_then_impl(*this, std::forward<F>(f));
        }
        template<typename F> requires returns_result_for_void<F>
        auto and_then(F&& f) && {
            return and_then_impl(std::move(*this), std::forward<F>(f));
        }
        template<typename F> requires returns_result_for_void<F>
        auto and_then(F&& f) const && {
            return and_then_impl(std::move(*this), std::forward<F>(f));
        }

        template<typename F> requires std::invocable<F, Error&>
        Result<void> map_err(F&& f) & {
            return map_err_impl(*this, std::forward<F>(f));
        }
        template<typename F> requires std::invocable<F, const Error&>
        Result<void> map_err(F&& f) const & {
            return map_err_impl(*this, std::forward<F>(f));
        }
        template<typename F> requires std::invocable<F, Error&&>
        Result<void> map_err(F&& f) && {
            return map_err_impl(std::move(*this), std::forward<F>(f));
        }
        template<typename F> requires std::invocable<F, const Error&&>
        Result<void> map_err(F&& f) const && {
            return map_err_impl(std::move(*this), std::forward<F>(f));
        }

        template<typename F> requires returns_result_for<F, Error&>
        Result<void> or_else(F&& f) & {
            return or_else_impl(*this, std::forward<F>(f));
        }
        template<typename F> requires returns_result_for<F, const Error&>
        Result<void> or_else(F&& f) const & {
            return or_else_impl(*this, std::forward<F>(f));
        }
        template<typename F> requires returns_result_for<F, Error&&>
        Result<void> or_else(F&& f) && {
            return or_else_impl(std::move(*this), std::forward<F>(f));
        }
        template<typename F> requires returns_result_for<F, const Error&&>
        Result<void> or_else(F&& f) const && {
            return or_else_impl(std::move(*this), std::forward<F>(f));
        }

        [[nodiscard]] std::optional<std::monostate> to_optional() const noexcept {
            return std::visit(
                [&]<typename T0>(T0 &&) -> std::optional<std::monostate> {
                    if constexpr (std::is_same_v<std::decay_t<T0>, std::monostate>) {
                        return std::monostate{};
                    } else {
                        return std::nullopt;
                    }
                },
              data_
            );
        }

    private:
        template<typename... Args>
        explicit Result(std::in_place_index_t<0>, Args&&... args) : data_(std::in_place_index<0>, std::forward<Args>(args)...) {}

        template<typename... Args>
        explicit Result(std::in_place_index_t<1>, Args&&... args) : data_(std::in_place_index<1>, std::forward<Args>(args)...) {}

        std::variant<std::monostate, Error> data_;

        template <typename Self, typename F>
        static auto map_impl(Self&& self, F&& f) {
            using ResultType = std::invoke_result_t<F>;

            return std::visit(
                [&]<typename T0>(T0&& arg) -> Result<ResultType> {
                    if constexpr (std::is_same_v<std::decay_t<T0>, std::monostate>) {
                        if constexpr (std::is_void_v<ResultType>) {
                            std::invoke(std::forward<F>(f));
                            return Result<void>::ok();
                        } else {
                            return Result<ResultType>::ok(std::invoke(std::forward<F>(f)));
                        }
                    } else {
                        return Result<ResultType>::err(std::forward<T0>(arg));
                    }
                },
                std::forward<Self>(self).data_
            );
        }

        template <typename Self, typename F>
        static auto and_then_impl(Self&& self, F&& f) {
            using ResultType = std::invoke_result_t<F>;

            return std::visit(
                [&]<typename T0>(T0&& arg) -> ResultType {
                    if constexpr (std::is_same_v<std::decay_t<T0>, std::monostate>) {
                      return std::invoke(std::forward<F>(f));
                    } else {
                        return ResultType::err(std::forward<T0>(arg));
                    }
                },
                std::forward<Self>(self).data_
            );
        }

        template <typename Self, typename F>
        static Result<void> map_err_impl(Self&& self, F&& f) {
            return std::visit(
                [&]<typename T0>(T0&& arg) -> Result<void> {
                    if constexpr (std::is_same_v<std::decay_t<T0>, std::monostate>) {
                        return Result<void>::ok();
                    } else {
                        using FResult = std::invoke_result_t<F, T0>;
                        if constexpr (std::is_same_v<FResult, Error>) {
                            return Result<void>::err(std::invoke(std::forward<F>(f), std::forward<T0>(arg)));
                        } else {
                          return Result<void>::err(Error{std::invoke(std::forward<F>(f), std::forward<T0>(arg))});
                        }
                    }
                },
                std::forward<Self>(self).data_
            );
        }

        template <typename Self, typename F>
        static Result<void> or_else_impl(Self&& self, F&& f) {
            return std::visit(
                [&]<typename T0>(T0&& arg) -> Result<void> {
                    if constexpr (std::is_same_v<std::decay_t<T0>, std::monostate>) {
                        return Result<void>::ok();
                    } else {
                        return std::invoke(std::forward<F>(f), std::forward<T0>(arg));
                    }
                },
                std::forward<Self>(self).data_
            );
        }
    };

    template<typename T>
    class Result {
    public:
        using value_type = T;
        using error_type = Error;

        [[nodiscard]] static Result<T> ok(T value) {
            return Result<T>(std::in_place_index<0>, std::move(value));
        }

        template<typename U>
            requires std::constructible_from<T, U>
        [[nodiscard]] static Result<T> ok(U&& value) {
            return Result<T>(std::in_place_index<0>, T(std::forward<U>(value)));
        }

        template<typename... Args>
            requires std::constructible_from<T, Args...>
        [[nodiscard]] static Result<T> ok_in_place(Args&&... args) {
          return Result<T>(std::in_place_index<0>, std::forward<Args>(args)...);
        }

        [[nodiscard]] static Result<T> err(const std::string &message, const int code = 0) {
            return Result<T>(std::in_place_index<1>, Error{message, code});
        }

        [[nodiscard]] static Result<T> err(Error error) {
            return Result<T>(std::in_place_index<1>, std::move(error));
        }

        // it will help if we add costume error type
        template<typename... EArgs>
            requires std::constructible_from<Error, EArgs...>
        [[nodiscard]] static Result<T> err_in_place(EArgs&&... args) {
          return Result<T>(std::in_place_index<1>, std::forward<EArgs>(args)...);
        }

        [[nodiscard]] bool is_ok() const { return std::holds_alternative<T>(data_);  }
        [[nodiscard]] bool is_err() const { return std::holds_alternative<Error>(data_); }
        explicit operator bool() const noexcept { return is_ok(); }

        T* operator->() & { return &unwrap(); }
        const T* operator->() const & { return &unwrap(); }

        T& operator*() & { return unwrap(); }
        const T& operator*() const & { return unwrap(); }
        T&& operator*() && { return std::move(unwrap()); }
        const T&& operator*() const && { return std::move(unwrap()); }

        T& expect(const std::string& msg) & {
            return std::visit(
                [&]<typename T0>(T0& arg) -> T& {
                    if constexpr (std::is_same_v<std::decay_t<T0>, T>) {
                        return arg;
                    } else {
                      throw std::runtime_error(msg + ": " + arg.message);
                    }
                }, data_);
        }
        const T& expect(const std::string& msg) const & {
            return std::visit(
                [&]<typename T0>(const T0& arg) -> const T& {
                    if constexpr (std::is_same_v<std::decay_t<T0>, T>) {
                        return arg;
                    } else {
                      throw std::runtime_error(msg + ": " + arg.message);
                    }
                }, data_);
        }
        T&& expect(const std::string& msg) && {
            return std::visit(
                [&]<typename T0>(T0&& arg) -> T&& {
                    if constexpr (std::is_same_v<std::decay_t<T0>, T>) {
                        return std::move(arg);
                    } else {
                      throw std::runtime_error(msg + ": " + arg.message);
                    }
                }, data_);
        }
        const T&& expect(const std::string& msg) const && {
            return std::visit(
                [&]<typename T0>(const T0&& arg) -> const T&& {
                    if constexpr (std::is_same_v<std::decay_t<T0>, T>) {
                        return std::move(arg);
                    } else {
                      throw std::runtime_error(msg + ": " + arg.message);
                    }
                }, data_);
        }

        T& unwrap() & {
            return expect("Attempted to unwrap error result");
        }
        const T& unwrap() const & {
            return expect("Attempted to unwrap error result");
        }
        T&& unwrap() && {
            return std::move(expect("Attempted to unwrap error result"));
        }
        const T&& unwrap() const && {
            return std::move(expect("Attempted to unwrap error result"));
        }

        // Removed unwrap_move(), which is replaced by T&& unwrap() &&

        [[nodiscard]] Error& unwrap_err() & {
            return std::visit(
                [&]<typename T0>(T0& arg) -> Error& {
                    if constexpr (std::is_same_v<std::decay_t<T0>, Error>) {
                        return arg;
                    } else {
                      throw std::runtime_error("Attempted to unwrap_err on ok result");
                    }
                },
            data_);
        }
        [[nodiscard]] const Error& unwrap_err() const & {
            return std::visit(
                [&]<typename T0>(const T0& arg) -> const Error& {
                    if constexpr (std::is_same_v<std::decay_t<T0>, Error>) {
                        return arg;
                    } else {
                      throw std::runtime_error("Attempted to unwrap_err on ok result");
                    }
                },
            data_);
        }
        [[nodiscard]] Error&& unwrap_err() && {
            return std::visit(
                [&]<typename T0>(T0&& arg) -> Error&& {
                    if constexpr (std::is_same_v<std::decay_t<T0>, Error>) {
                        return std::move(arg);
                    } else {
                      throw std::runtime_error("Attempted to unwrap_err on ok result");
                    }
                },
            data_);
        }
        [[nodiscard]] const Error&& unwrap_err() const && {
            return std::visit(
                [&]<typename T0>(const T0&& arg) -> const Error&& {
                    if constexpr (std::is_same_v<std::decay_t<T0>, Error>) {
                        return std::move(arg);
                    } else {
                      throw std::runtime_error("Attempted to unwrap_err on ok result");
                    }
                },
            data_);
        }

        template<typename FOk, typename FErr>
            requires std::invocable<FOk, T&> &&
                     std::invocable<FErr, Error&> &&
                     std::is_same_v<std::invoke_result_t<FOk, T&>, std::invoke_result_t<FErr, Error&>>
        auto match(FOk&& ok_fn, FErr&& err_fn) & {
            return match_impl(*this, std::forward<FOk>(ok_fn), std::forward<FErr>(err_fn));
        }
        template<typename FOk, typename FErr>
            requires std::invocable<FOk, const T&> &&
                     std::invocable<FErr, const Error&> &&
                     std::is_same_v<std::invoke_result_t<FOk, const T&>, std::invoke_result_t<FErr, const Error&>>
        auto match(FOk&& ok_fn, FErr&& err_fn) const & {
            return match_impl(*this, std::forward<FOk>(ok_fn), std::forward<FErr>(err_fn));
        }
        template<typename FOk, typename FErr>
            requires std::invocable<FOk, T&&> &&
                     std::invocable<FErr, Error&&> &&
                     std::is_same_v<std::invoke_result_t<FOk, T&&>, std::invoke_result_t<FErr, Error&&>>
        auto match(FOk&& ok_fn, FErr&& err_fn) && {
            return match_impl(*this, std::forward<FOk>(ok_fn), std::forward<FErr>(err_fn));
        }
        template<typename FOk, typename FErr>
            requires std::invocable<FOk, const T&&> &&
                     std::invocable<FErr, const Error&&> &&
                     std::is_same_v<std::invoke_result_t<FOk, const T&&>, std::invoke_result_t<FErr, const Error&&>>
        auto match(FOk&& ok_fn, FErr&& err_fn) const && {
            return match_impl(*this, std::forward<FOk>(ok_fn), std::forward<FErr>(err_fn));
        }

        [[nodiscard]] T unwrap_or(const T& default_val) const & {
            if(is_ok()) return std::get<T>(data_);
            return default_val;
        }
        [[nodiscard]] T unwrap_or(T&& default_val) && {
            if(is_ok()) return std::move(std::get<T>(data_));
            return std::move(default_val);
        }

        template<typename F> requires std::invocable<F, Error&> && std::is_same_v<std::invoke_result_t<F, Error&>, T>
        T unwrap_or_else(F&& f) & {
            if (is_ok()) return std::get<T>(data_);
            return std::invoke(std::forward<F>(f), std::get<Error>(data_));
        }
        template<typename F> requires std::invocable<F, const Error&> && std::is_same_v<std::invoke_result_t<F, const Error&>, T>
        T unwrap_or_else(F&& f) const & {
            if (is_ok()) return std::get<T>(data_);
            return std::invoke(std::forward<F>(f), std::get<Error>(data_));
        }
        template<typename F> requires std::invocable<F, Error&&> && std::is_same_v<std::invoke_result_t<F, Error&&>, T>
        T unwrap_or_else(F&& f) && {
            if (is_ok()) return std::move(std::get<T>(data_));
            return std::invoke(std::forward<F>(f), std::move(std::get<Error>(data_)));
        }
        template<typename F> requires std::invocable<F, const Error&&> && std::is_same_v<std::invoke_result_t<F, const Error&&>, T>
        T unwrap_or_else(F&& f) const && {
            if (is_ok()) return std::move(std::get<T>(data_));
            return std::invoke(std::forward<F>(f), std::move(std::get<Error>(data_)));
        }

        [[nodiscard]] T* try_unwrap() & noexcept {
            if(is_ok()) return &std::get<T>(data_);
            return nullptr;
        }
        [[nodiscard]] const T* try_unwrap() const & noexcept {
            if(is_ok()) return &std::get<T>(data_);
            return nullptr;
        }

        [[nodiscard]] Error& expect_err(const std::string& msg) & {
            if (is_err()) return std::get<Error>(data_);
            throw std::runtime_error(msg);
        }
        [[nodiscard]] const Error& expect_err(const std::string& msg) const & {
            if (is_err()) return std::get<Error>(data_);
            throw std::runtime_error(msg);
        }
        [[nodiscard]] Error&& expect_err(const std::string& msg) && {
            if (is_err()) return std::move(std::get<Error>(data_));
            throw std::runtime_error(msg);
        }
        [[nodiscard]] const Error&& expect_err(const std::string& msg) const && {
            if (is_err()) return std::move(std::get<Error>(data_));
            throw std::runtime_error(msg);
        }

        template<typename F> requires returns_result_for<F, T&>
        auto and_then(F&& f) & {
            return and_then_impl(*this, std::forward<F>(f));
        }
        template<typename F> requires returns_result_for<F, const T&>
        auto and_then(F&& f) const & {
            return and_then_impl(*this, std::forward<F>(f));
        }
        template<typename F> requires returns_result_for<F, T&&>
        auto and_then(F&& f) && {
            return and_then_impl(std::move(*this), std::forward<F>(f));
        }
        template<typename F> requires returns_result_for<F, const T&&>
        auto and_then(F&& f) const && {
            return and_then_impl(std::move(*this), std::forward<F>(f));
        }

        template<std::invocable<T&> F>
        auto map(F&& f) & {
            return map_impl(*this, std::forward<F>(f));
        }
        template<std::invocable<const T&> F>
        auto map(F&& f) const & {
            return map_impl(*this, std::forward<F>(f));
        }
        template<std::invocable<T&&> F>
        auto map(F&& f) && {
            return map_impl(std::move(*this), std::forward<F>(f));
        }
        template<std::invocable<const T&&> F>
        auto map(F&& f) const && {
            return map_impl(std::move(*this), std::forward<F>(f));
        }

        template<std::invocable<Error&> F>
        Result<T> map_err(F&& f) & {
            return map_err_impl(*this, std::forward<F>(f));
        }
        template<std::invocable<const Error&> F>
        Result<T> map_err(F&& f) const & {
            return map_err_impl(*this, std::forward<F>(f));
        }
        template<std::invocable<Error&&> F>
        Result<T> map_err(F&& f) && {
            return map_err_impl(std::move(*this), std::forward<F>(f));
        }
        template<std::invocable<const Error&&> F>
        Result<T> map_err(F&& f) const && {
            return map_err_impl(std::move(*this), std::forward<F>(f));
        }

        template<typename F> requires returns_result_for<F, Error&>
        Result<T> or_else(F&& f) & {
            return or_else_impl(*this, std::forward<F>(f));
        }
        template<typename F> requires returns_result_for<F, const Error&>
        Result<T> or_else(F&& f) const & {
            return or_else_impl(*this, std::forward<F>(f));
        }
        template<typename F> requires returns_result_for<F, Error&&>
        Result<T> or_else(F&& f) && {
            return or_else_impl(std::move(*this), std::forward<F>(f));
        }
        template<typename F> requires returns_result_for<F, const Error&&>
        Result<T> or_else(F&& f) const && {
            return or_else_impl(std::move(*this), std::forward<F>(f));
        }

        template<typename U>
        bool contains(const U& value) const noexcept {
            if(is_ok()) return std::get<T>(data_) == value;
            return false;
        }

        [[nodiscard]] std::optional<T> to_optional() const & noexcept {
            if(is_ok()) return std::get<T>(data_);
            return std::nullopt;
        }
        [[nodiscard]] std::optional<T> to_optional() && noexcept {
            if(is_ok()) return std::move(std::get<T>(data_));
            return std::nullopt;
        }

    private:
        // Private constructors for explicit in_place construction
        template<typename... Args>
        explicit Result(std::in_place_index_t<0>, Args&&... args) : data_(std::in_place_index<0>, std::forward<Args>(args)...) {}

        template<typename... Args>
        explicit Result(std::in_place_index_t<1>, Args&&... args) : data_(std::in_place_index<1>, std::forward<Args>(args)...) {}

        std::variant<T, Error> data_;

        template <typename Self, typename FOk, typename FErr>
        static auto match_impl(Self&& self, FOk&& ok_fn, FErr&& err_fn) {
            using ResultType = std::invoke_result_t<FOk, decltype(std::forward<Self>(self).unwrap())>;

            return std::visit(
                [&]<typename T0>(T0&& arg) -> ResultType {
                    if constexpr (std::is_same_v<std::decay_t<T0>, T>) {
                        return std::invoke(std::forward<FOk>(ok_fn), std::forward<T0>(arg));
                    } else {
                        return std::invoke(std::forward<FErr>(err_fn), std::forward<T0>(arg));
                    }
                }, std::forward<Self>(self).data_);
        }

        template <typename Self, typename F>
        static auto map_impl(Self&& self, F&& f) {
            using ResultType = std::invoke_result_t<F, decltype(std::forward<Self>(self).unwrap())>;

            return std::visit(
                [&]<typename T0>(T0&& arg) -> Result<ResultType> {
                    if constexpr (std::is_same_v<std::decay_t<T0>, T>) {
                        if constexpr (std::is_void_v<ResultType>) {
                          std::invoke(std::forward<F>(f), std::forward<T0>(arg));
                          return Result<void>::ok();
                        } else {
                            return Result<ResultType>::ok(
                                std::invoke(std::forward<F>(f), std::forward<T0>(arg))
                            );
                        }
                    } else {
                        return Result<ResultType>::err(std::forward<T0>(arg));
                    }
                }, std::forward<Self>(self).data_);
        }

        template <typename Self, typename F>
        static auto and_then_impl(Self&& self, F&& f) {
            using ResultType = std::invoke_result_t<F, decltype(std::forward<Self>(self).unwrap())>;

            return std::visit(
                [&]<typename T0>(T0&& arg) -> ResultType {
                    if constexpr (std::is_same_v<std::decay_t<T0>, T>) {
                        return std::invoke(std::forward<F>(f), std::forward<T0>(arg));
                    } else {
                        return ResultType::err(std::forward<T0>(arg));
                    }
                }, std::forward<Self>(self).data_);
        }

        template <typename Self, typename F>
        static Result<T> map_err_impl(Self&& self, F&& f) {
            return std::visit(
                [&]<typename T0>(T0&& arg) -> Result<T> {
                    if constexpr (std::is_same_v<std::decay_t<T0>, T>) {
                        return Result<T>::ok(std::forward<T0>(arg));
                    } else {
                        using FResult = std::invoke_result_t<F, T0>;
                        if constexpr (std::is_same_v<FResult, Error>) {
                            return Result<T>::err(std::invoke(std::forward<F>(f), std::forward<T0>(arg)));
                        } else {
                            return Result<T>::err(Error{std::invoke(std::forward<F>(f), std::forward<T0>(arg))});
                        }
                    }
                }, std::forward<Self>(self).data_);
        }

        template <typename Self, typename F>
        static Result<T> or_else_impl(Self&& self, F&& f) {
            return std::visit(
                [&]<typename T0>(T0&& arg) -> Result<T> {
                    if constexpr (std::is_same_v<std::decay_t<T0>, T>) {
                        return Result<T>::ok(std::forward<T0>(arg));
                    } else {
                        return std::invoke(std::forward<F>(f), std::forward<T0>(arg));
                    }
                }, std::forward<Self>(self).data_);
        }
    };
} // namespace neonfs
