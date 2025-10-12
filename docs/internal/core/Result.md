# Result<T> — Error Handling Wrapper

---
namespace:
- `neonfs`
---

## Why `Result<T>`?

`Result<T>` is a modern C++ approach to explicit error handling inspired by Rust's `Result` type. It provides a robust and expressive way for functions to return either:

- A **success value** (`ok`) of type `T`.
- An **error** (`err`) object containing a message and an optional error code.

This pattern eliminates the ambiguity of raw pointers, integer codes, or exceptions for standard error flow, leading to safer and more readable code.

## Creating a `Result`

You can create a `Result` in one of two states:

### Success: `ok`

The `ok()` static method creates a success result. It can construct the value in-place, simplifying usage.

```cpp
// Creates a Result<int> holding the value 10
auto success = neonfs::Result<int>::ok(10);

// In-place construction also works
auto text = neonfs::Result<std::string>::ok("hello world");
```

### Error: `err`

The `err()` static method creates an error result, taking a message and an optional integer code.

```cpp
// Creates an error Result<int>
auto error = neonfs::Result<int>::err("Something went wrong", 1);
```

## Checking the State

To safely handle a `Result`, first check if it contains a value or an error.

- `is_ok()`: Returns `true` if the result is a success.
- `is_err()`: Returns `true` if the result is an error.
- `operator bool`: The `Result` object can be used in a boolean context for a concise check.

```cpp
auto result = neonfs::Result<int>::ok(42);

if (result.is_ok()) {
    // ...
}

if (result) { // a more idiomatic way
    // ...
}
```

## Accessing the Value or Error

### Unwrapping

Unwrapping extracts the underlying value or error. Be cautious, as unwrapping the wrong state will throw a `std::runtime_error`.

- `unwrap()`: Returns a reference to the success value. Throws if it's an error.
- `unwrap_err()`: Returns a const reference to the error. Throws if it's a success.
- `unwrap_move()`: Moves the success value out of the `Result`. Throws if it's an error.
- `expect_err(msg)`: Returns the error, but throws a `std::runtime_error` with a custom message if it's a success.

### Safe Access with Defaults

- `unwrap_or(default_val)`: Returns the success value or a provided default if it's an error.
- `unwrap_or_else(fn)`: Returns the success value or computes a value by passing the error to the provided function.
- `try_unwrap()`: Returns an `std::optional<std::reference_wrapper<T>>` to the success value, or `std::nullopt` for an error.

```cpp
auto err_result = neonfs::Result<std::string>::err("file not found");

// Returns "default.txt"
auto path = err_result.unwrap_or("default.txt");

// Computes a fallback value from the error
auto fallback = err_result.unwrap_or_else([](const neonfs::Error& e) {
    return std::string("fallback for: ") + e.message;
});
```

### Pointer-like Access

For convenience, you can access the underlying value's members directly using pointer-like syntax. This will throw an exception if the result is an error.

- `operator->()`: Access members of the contained value.
- `operator*()`: Dereference to get the contained value.

```cpp
struct User { std::string name; };
auto result = neonfs::Result<User>::ok({"John"});

if (result) {
    // Both lines are equivalent and print "John"
    std::cout << result->name << std::endl;
    std::cout << (*result).name << std::endl;
}
```

## Combinators and Chaining

Combinators allow you to transform and chain `Result` objects in a clean, functional style.

- `map(fn)`: If the result is `ok`, applies `fn` to the success value and returns a new `Result` with the transformed value. If the result is `err`, it propagates the error. Handles functions returning `void`.
- `map_err(fn)`: If the result is `err`, applies `fn` to the `Error` object to transform the error.
- `and_then(fn)`: If the result is `ok`, calls `fn` with the success value. `fn` must return a `Result`. This is useful for chaining operations that can each fail.
- `or_else(fn)`: If the result is `err`, calls `fn` with the `Error` object. `fn` must return a `Result`. This allows you to attempt a recovery or an alternative operation.

```cpp
neonfs::Result<int> to_int(const std::string& s) {
    try {
        return neonfs::Result<int>::ok(std::stoi(s));
    } catch (...) {
        return neonfs::Result<int>::err("conversion failed");
    }
}

auto result = neonfs::Result<std::string>::ok("123")
    .and_then(to_int)         // -> Result<int>::ok(123)
    .map([](int i) { return i * 2; }); // -> Result<int>::ok(246)

// Final value is 246
int final_val = result.unwrap();
```

## Other Utilities

- `match(ok_fn, err_fn)`: Executes `ok_fn` on a success value or `err_fn` on an error, returning the result of the executed function. Both functions must return the same type.
- `contains(value)`: Returns `true` if the result is `ok` and its value is equal to the provided `value`.
- `to_optional()`: Converts a `Result<T>` to an `std::optional<T>`, discarding the error information.

## `Result<void>` Specialization

A specialization `Result<void>` is provided for operations that either succeed without returning a value or fail with an error. It has a similar set of methods but is tailored for the `void` type.

---

For detailed usage patterns and examples, see the [Result Usage](ResultUsage.md) documentation.