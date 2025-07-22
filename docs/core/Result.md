# Result<T> — Error Handling Wrapper

---
namespace:
- `neonfs`
---

## Why `Result<T>`?

`Result<T>` is a modern C++ approach to explicit error handling inspired by Rust's `Result` type. It allows functions that may fail to return either:

- A **success value** (`ok`)
- An **error message** (`err`)

This avoids exceptions and unsafe nulls or special values, forcing callers to handle both cases explicitly.

---

## What Problems Does It Solve?

- Avoids ambiguity of special return values (e.g., nullptr, -1).
- Provides explicit and consistent error reporting.
- Facilitates chaining operations without nested error checks.
- Prevents silent errors by forcing explicit unwrap or error handling.
- Simplifies interaction with foreign languages (e.g., JavaScript) that don't support C++ exceptions well.

---

## How Does It Work?

`Result<T>` wraps either:

- A **value** of type `T` indicating success, or
- Or an `Error` struct:

```cpp
struct Error {
    std::string message;
    int code = 0;
};
```
> We use a real Error struct instead of just strings to provide structured and machine-readable error information.

Key Methods:
- `Result<T>::ok(value)` — returns a success result.
- `Result<T>::err("message", errno)` — returns a failure with error message and code.
- `is_ok()` / `is_err()` — query the state.
- `unwrap()` / `unwrap_err()` — extract the result or error.
- `expect_err("message")` — unwraps an error, throwing a custom message if the result is ok.
- `try_unwrap()` — returns the value as an optional.
- `and_then(f)` — chains operations that may return Result.
- Specialized for void as well:

```cpp
Result<void>::ok()
Result<void>::err("something went wrong", 123)
```
---

## When to Use `Result<T>`

Use `Result<T>` in any function that can fail, especially:

- File I/O operations
- Network requests
- Parsing and validation
- Interactions with native bindings in Electron or other languages

### Example

```cpp
Result<std::string> read_file(const std::string& path);

auto res = read_file("config.txt");
if (res.is_err()) {
    Error e = res.unwrap_err();
    std::cerr << "Error reading file: " << e.message << " (code " << e.code << ")\n";
} else {
    std::string data = res.unwrap();
    // use data...
}
```

---

## Benefits of Using `Result<T>`

* Explicit and clear control flow.
* Eliminates silent failures and runtime surprises.
* Easier to maintain and reason about error handling.
* Compatible with JavaScript native addons requiring manual error handling.

---

## Additional Notes

* `Result<void>` specialization handles operations with no meaningful return value.
* Exceptions are thrown only when `unwrap()`/`unwrap_err()` is misused.
* The API supports functional chaining for cleaner, concise code.

---

For more examples and integration details, see the [Result Usage](ResultUsage.md) documentation.