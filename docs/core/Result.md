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

- **Type Safety**: Eliminates ambiguity by encoding success or failure directly into the return type (e.g., nullptr, -1).
- **Explicit Handling**: Forces callers to handle potential errors, preventing silent failures.
- **Efficiency**: Uses `std::variant` to store either the value or the error, avoiding the cost of exceptions and the memory overhead of storing an unused value in error cases.
- **Composability**: Provides functional helpers to chain operations cleanly without nested `if/else` blocks.
- **Cross-Language Compatibility**: Simplifies interaction with foreign languages (like JavaScript via N-API) that do not support C++ exceptions.

---

## How Does It Work?

`Result<T>` is a wrapper around `std::variant<T, Error>`. It holds either:

- A **value** of type `T` indicating success, or
- An `Error` struct indicating failure:

```cpp
struct Error {
    std::string message;
    int code = 0;
};
```

> Using a `std::variant` is highly efficient, as it only allocates storage for the type it currently holds. For `Result<void>`, it uses `std::monostate` to represent the "ok" state with no value.

Key Methods:
- `Result<T>::ok(value)` / `Result<void>::ok()` — Creates a success result.
- `Result<T>::err(...)` — Creates a failure result.
- `is_ok()` / `is_err()` — Checks the state of the result.
- `unwrap()` / `unwrap_err()` — Extracts the value or error, throwing an exception if the state is incorrect.
- `match(ok_fn, err_fn)` — Handles both states with corresponding lambdas in a single call.
- `map(f)` / `map_err(f)` — Transforms the success or error value.
- `and_then(f)` / `or_else(f)` — Chains operations that return a `Result`.
- `unwrap_or(default)` / `unwrap_or_else(f)` — Returns the success value or a default.

---

## When to Use `Result<T>`

Use `Result<T>` as the return type for any function that can fail, especially:

- File I/O operations
- Network requests
- Data parsing and validation
- Any operation that crosses an API boundary (e.g., native addon to JavaScript).

### Example

```cpp
Result<std::string> read_file(const std::string& path);

auto res = read_file("config.txt");

// Use the powerful match function to handle both cases
res.match(
    [](const std::string& data) {
        std::cout << "File content: " << data << std::endl;
    },
    [](const Error& e) {
        std::cerr << "Error: " << e.message << " (code " << e.code << ")\n";
    }
);
```

---

## Benefits of Using `Result<T>`

- **Robustness**: Makes your code less prone to bugs from unhandled errors.
- **Clarity**: The function signature `Result<T>` immediately tells the caller that the operation can fail.
- **Maintainability**: Error handling logic is clean, local, and easy to follow.

---

For detailed usage patterns and examples, see the [Result Usage](ResultUsage.md) documentation.