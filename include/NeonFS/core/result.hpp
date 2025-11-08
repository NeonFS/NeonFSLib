#pragma once
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <functional>

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
        explicit operator bool() const noexcept { return is_ok(); }

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

        template<typename F> requires std::invocable<F, Error> && std::is_void_v<std::invoke_result_t<F, Error>>
        void unwrap_or_else(F&& f) {
            return std::visit(
                [&]<typename T0>(T0&& arg) -> void {
                    using Arg = std::decay_t<T0>;

                    if constexpr (std::is_same_v<Arg, Error>) {
                        std::invoke(std::forward<F>(f), arg);
                    }
                },
            data_);
        }

        [[nodiscard]] std::optional<std::monostate> try_unwrap() const noexcept {
            return std::visit(
                [&]<typename T0>(T0 &&) -> std::optional<std::monostate> {
                    using Arg = std::decay_t<T0>;
                    if constexpr (std::is_same_v<Arg, Error>) {
                        return std::nullopt;
                    } else {
                        return std::monostate{};
                    }
                },
                data_
            );
        }

        [[nodiscard]] const Error& unwrap_err() const {
            return std::visit(
                [&]<typename T0>(T0&& arg) -> const Error& {
                    using Arg = std::decay_t<T0>;

                    if constexpr (std::is_same_v<Arg, Error>) {
                        return arg;
                    } else {
                        throw std::runtime_error("Attempted to unwrap_err on ok result");
                    }
                },
            data_);
        }

        [[nodiscard]] const Error& expect_err(const std::string& msg) const {
            return std::visit(
                  [&]<typename T0>(T0&& arg) -> const Error& {
                  using Arg = std::decay_t<T0>;

                  if constexpr (std::is_same_v<Arg, Error>) {
                      return arg;
                  } else {
                      throw std::runtime_error(msg);
                  }
            },
            data_);
        }

        template<typename FOk, typename FErr>
          requires std::invocable<FOk> &&
              std::invocable<FErr, Error> &&
              std::is_same_v<std::invoke_result_t<FOk>, std::invoke_result_t<FErr, Error>>
        auto match(FOk&& ok_fn, FErr&& err_fn) {
            using ResultType = std::invoke_result_t<FOk>;

            return std::visit(
                [&]<typename T0>(T0&& arg) -> ResultType {
                    using Arg = std::decay_t<T0>;

                    if constexpr (std::is_same_v<Arg, std::monostate>) {
                        return std::invoke(std::forward<FOk>(ok_fn));
                    } else {
                        return std::invoke(std::forward<FErr>(err_fn), arg);
                    }
                },
                data_
            );
        }

        template<returns_result_for_void F>
        auto and_then(F&& f) const {
            using ResultType = std::invoke_result_t<F>;

            return std::visit(
                [&]<typename T0>(T0&& arg) -> ResultType {
                    using Arg = std::decay_t<T0>;

                    if constexpr (std::is_same_v<Arg, std::monostate>) {
                        return std::invoke(std::forward<F>(f));
                    } else {
                        return ResultType::err(arg);
                    }
                },
                data_
            );
        }

        template<std::invocable F>
        auto map(F&& f) {
            using ResultType = std::invoke_result_t<F>;

            return std::visit(
                [&]<typename T0>(T0&& arg) -> Result<ResultType> {
                    using Arg = std::decay_t<T0>;

                    if constexpr (std::is_same_v<Arg, std::monostate>) {
                        if constexpr (std::is_void_v<ResultType>) {
                            std::invoke(std::forward<F>(f));
                            return Result<void>::ok();
                        } else {
                            return Result<ResultType>::ok(
                                std::invoke(std::forward<F>(f))
                            );
                        }
                    } else {
                        // Forward the error without copying
                        return Result<ResultType>::err(std::forward<T0>(arg));
                    }
                },
                data_
            );
        }

        template<std::invocable<Error> F>
        Result<void> map_err(F&& f) const {
            return std::visit(
                [&]<typename T0>(T0&& arg) -> Result<void> {
                    using Arg = std::decay_t<T0>;

                    if constexpr (std::is_same_v<Arg, std::monostate>) {
                        return *this;
                    } else {
                        if constexpr (std::is_same_v<std::invoke_result_t<F, Error>, Error>) {
                            return Result<void>::err(
                                std::invoke(std::forward<F>(f), std::forward<T0>(arg))
                            );
                        } else {
                            // if f return something convertible to Error
                            return Result<void>::err(
                                Error{std::invoke(std::forward<F>(f), arg)}
                            );
                        }
                    }
                },
                data_
            );
        }

        template<returns_result_for<Error> F>
        Result<void> or_else(F&& f) const {
            return std::visit(
                [&]<typename T0>(T0&& arg) -> Result<void> {
                    using Arg = std::decay_t<T0>;

                    if constexpr (std::is_same_v<Arg, std::monostate>) {
                        return *this;
                    } else {
                        return std::invoke(std::forward<F>(f), arg);
                    }
                },
                data_
            );
        }

        [[nodiscard]] std::optional<std::monostate> to_optional() const noexcept {
            return std::visit(
                [&]<typename T0>(T0 &&) -> std::optional<std::monostate> {
                    using Arg = std::decay_t<T0>;

                    if constexpr (std::is_same_v<Arg, std::monostate>) {
                        return std::monostate{};
                    } else {
                        return std::nullopt;
                    }
                },
              data_
            );
        }

    private:
        explicit Result(std::monostate) : data_(std::monostate{}) {}
        explicit Result(Error error) : data_(std::move(error)) {}

        std::variant<std::monostate, Error> data_;
    };

    template<typename T>
    class Result {
    public:
        using value_type = T;
        using error_type = Error;

        [[nodiscard]] static Result<T> ok(T value) {
            return Result<T>(std::move(value));
        }

        // allowing implicit construction of T inside ok(),
        // Result<std::string>::ok("hi") works without writing std::string("hi").
        template<typename U>
        [[nodiscard]] static Result<T> ok(U&& value) {
            return Result<T>(T(std::forward<U>(value)));
        }

        [[nodiscard]] static Result<T> err(const std::string &message, const int code = 0) {
            return Result<T>(Error{message, code});
        }

        [[nodiscard]] static Result<T> err(Error error) {
            return Result<T>(std::move(error));
        }

        [[nodiscard]] bool is_ok() const { return std::holds_alternative<T>(data_);  }
        [[nodiscard]] bool is_err() const { return std::holds_alternative<Error>(data_); }
        explicit operator bool() const noexcept { return is_ok(); }

        T* operator->() { return &unwrap(); }
        const T* operator->() const { return &unwrap(); }

        T& operator*() & { return unwrap(); }
        const T& operator*() const & { return unwrap(); }
        T&& operator*() && { return std::move(unwrap()); }

        T& unwrap() {
            return std::visit(
                [&]<typename T0>(T0&& arg) -> T& {
                    using Arg = std::decay_t<T0>;

                    if constexpr (std::is_same_v<Arg, T>) {
                        return arg;
                    } else {
                      throw std::runtime_error("Attempted to unwrap error result: " + arg.message);
                    }
                },
            data_);
        }

        [[nodiscard]] const T& unwrap() const {
            return std::visit(
              [&]<typename T0>(T0&& arg) -> const T& {
                  using Arg = std::decay_t<T0>;

                  if constexpr (std::is_same_v<Arg, T>) {
                      return arg;
                  } else {
                    throw std::runtime_error("Attempted to unwrap error result: " + arg.message);
                  }
              },
            data_);
        }

        T unwrap_move() const {
            return std::visit(
              [&]<typename T0>(T0&& arg) -> T {
                  using Arg = std::decay_t<T0>;

                  if constexpr (std::is_same_v<Arg, T>) {
                      return std::move(std::forward<T0>(arg));
                  } else {
                    throw std::runtime_error("Attempted to unwrap error result: " + arg.message);
                  }
              },
            data_);
        }

        [[nodiscard]] const Error& unwrap_err() const {
            return std::visit(
              [&]<typename T0>(T0&& arg) -> const Error& {
                  using Arg = std::decay_t<T0>;

                  if constexpr (std::is_same_v<Arg, Error>) {
                      return arg;
                  } else {
                    throw std::runtime_error("Attempted to unwrap_err on ok result");
                  }
              },
            data_);
        }

        template<typename FOk, typename FErr>
            requires std::invocable<FOk, T> &&
                std::invocable<FErr, Error> &&
                std::is_same_v<std::invoke_result_t<FOk, T>, std::invoke_result_t<FErr, Error>>
        auto match(FOk&& ok_fn, FErr&& err_fn) const {
            using ResultType = std::invoke_result_t<FOk, T>;

            return std::visit(
                [&]<typename T0>(T0&& arg) -> ResultType {  // Explicit return type
                    using Arg = std::decay_t<T0>;
                    if constexpr (std::is_same_v<Arg, T>) {
                        return std::invoke(std::forward<FOk>(ok_fn), std::forward<T0>(arg));
                    } else {
                        return std::invoke(std::forward<FErr>(err_fn), std::forward<T0>(arg));
                    }
                },
                data_
            );
        }

        [[nodiscard]] T unwrap_or(const T& default_val) const {
            return std::visit(
              [&]<typename T0>(T0&& arg) -> T {
                  using Arg = std::decay_t<T0>;

                  if constexpr (std::is_same_v<Arg, T>) {
                      return arg;
                  } else {
                      return default_val;
                  }
              },
              data_
            );
        }

        template<typename F> requires std::invocable<F, Error> && std::is_same_v<std::invoke_result_t<F, Error>, T>
        T unwrap_or_else(F&& f) const {
            return std::visit(
                [&]<typename T0>(T0&& arg) -> T {
                    using Arg = std::decay_t<T0>;

                    if constexpr (std::is_same_v<Arg, Error>) {
                        return std::invoke(std::forward<F>(f), arg);
                    } else {
                        return arg;
                    }
                },
            data_);
        }

        std::optional<std::reference_wrapper<T>> try_unwrap() noexcept {
            return std::visit(
              [&]<typename T0>(T0&& arg) -> std::optional<std::reference_wrapper<T>> {
                using Arg = std::decay_t<T0>;
                if constexpr (std::is_same_v<Arg, T>) {
                  return std::ref(std::forward<T0>(arg));
                } else {
                  return std::nullopt;
                }
              },
              data_);
        }

        [[nodiscard]] const Error& expect_err(const std::string& msg) const {
            return std::visit(
              [&]<typename T0>(T0&& arg) -> const Error& {
                using Arg = std::decay_t<T0>;

                if constexpr (std::is_same_v<Arg, Error>) {
                    return arg;
                } else {
                    throw std::runtime_error(msg);
                }
            },
            data_);
        }

        template<returns_result_for<T> F>
        auto and_then(F&& f) const {
            using ResultType = std::invoke_result_t<F, T>;

            return std::visit(
                [&]<typename T0>(T0&& arg) -> ResultType {
                    using Arg = std::decay_t<T0>;

                    if constexpr (std::is_same_v<Arg, T>) {
                        return std::invoke(std::forward<F>(f), std::forward<T0>(arg));
                    } else {
                        return ResultType::err(std::forward<T0>(arg));
                    }
                },
                data_
            );
        }

        template<std::invocable<T> F>
        auto map(F&& f) const {
            using ResultType = std::invoke_result_t<F, T>;

            return std::visit(
              [&]<typename T0>(T0&& arg) -> Result<ResultType> {
                  using Arg = std::decay_t<T0>;

                  if constexpr (std::is_same_v<Arg, T>) {
                      if constexpr (std::is_void_v<ResultType>) {
                        std::invoke(std::forward<F>(f), std::forward<T0>(arg));
                        return Result<void>::ok();
                      } else {
                          return Result<ResultType>::ok(
                              std::invoke(std::forward<F>(f), std::forward<T0>(arg))
                          );
                      }
                  } else {
                      // Forward the error without copying
                      return Result<ResultType>::err(std::forward<T0>(arg));
                  }
              },
              data_);
        }

        template<std::invocable<Error> F>
        Result<T> map_err(F&& f) const {
            return std::visit(
                [&]<typename T0>(T0&& arg) -> Result<T> {
                    using Arg = std::decay_t<T0>;

                    if constexpr (std::is_same_v<Arg, T>) {
                        return *this;
                    } else {
                        if constexpr (std::is_same_v<std::invoke_result_t<F, Error>, Error>) {
                            return Result<T>::err(
                                std::invoke(std::forward<F>(f), std::forward<T0>(arg))
                            );
                        } else {
                            // if f return something convertible to Error
                            return Result<T>::err(
                                Error{std::invoke(std::forward<F>(f), std::forward<T0>(arg))}
                            );
                        }
                    }
                },
                data_
            );
        }

        template<returns_result_for<Error> F>
        Result<T> or_else(F&& f) const {
            return std::visit(
                [&]<typename T0>(T0&& arg) -> Result<T> {
                    using Arg = std::decay_t<T0>;

                    if constexpr (std::is_same_v<Arg, T>) {
                        return *this;
                    } else {
                        return std::invoke(std::forward<F>(f), std::forward<T0>(arg));
                    }
                },
                data_
            );
        }

        template<typename U>
        bool contains(const U& value) const noexcept {
            return std::visit(
              [&]<typename T0>(T0&& arg) -> bool {
                using Arg = std::decay_t<T0>;

                if constexpr (std::is_same_v<Arg, T>) {
                  return arg == value;
                } else {
                  return false;
                }
              },
              data_
            );
        }

        // Transform to std::optional
        [[nodiscard]] std::optional<T> to_optional() const noexcept {
            return std::visit(
              [&]<typename T0>(T0&& arg) -> std::optional<T> {
                using Arg = std::decay_t<T0>;

                if constexpr (std::is_same_v<Arg, T>) {
                  return arg;
                } else {
                  return std::nullopt;
                }
              },
              data_
            );
        }

    private:
        explicit Result(T value) : data_(std::move(value)) {}
        explicit Result(Error error) : data_(std::move(error)) {}

        std::variant<T, Error> data_;
    };
} // namespace neonfs
