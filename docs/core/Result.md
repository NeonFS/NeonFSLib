# Result<T> — Error Handling Wrapper

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
- An **error string** describing the failure.

It provides:

- `ok(value)` and `err(message)` static methods to create results.
- Methods like `is_ok()`, `is_err()`, `unwrap()`, and `unwrap_err()` to query and extract values.
- Functional helpers like `and_then()`, `map()`, and `or_else()` to compose operations safely.
- A specialization `Result<void>` for functions that only succeed or fail without a return value.

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

auto result = read_file("config.json");
if (result.is_err()) {
    // Handle error
    std::cerr << "Failed to read file: " << result.unwrap_err() << "\n";
} else {
    std::string contents = result.unwrap();
    // Use contents...
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
* Exceptions are thrown only when `unwrap()` is misused on an error result.
* The API supports functional chaining for cleaner, concise code.

---

For more examples and integration details, see the [Usage](Usage.md) documentation.