# Usage of `Result<T>`

This document explains how to effectively use the `Result<T>` type with practical examples.

---

## Creating a `Result`

All operations start by creating a `Result` to represent either success (`ok`) or failure (`err`).

-   **`Result<T>::ok(value)`**: Creates a success result containing a value.
-   **`Result<void>::ok()`**: Creates a success result indicating an operation completed with no return value.
-   **`Result<T>::err(message, code)`**: Creates a failure result with an error message and an optional error code.

```cpp
// Success with a value
auto success = Result ::ok(100);
// Success with no value
auto void_success = Result ::ok();
// Failure with a message and system error code
auto failure = Result ::err("Could not read from socket", errno);
```

## Returning and Handling a `Result`

### Returning a Result

Functions that can fail should return a `Result<T>`.

```cpp
Result<std::string> read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return Result<std::string>::err("Failed to open file", errno);
    }
    std::string contents((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return Result<std::string>::ok(std::move(contents));
}
````

### Handling Outcomes with `match` (Preferred Method)

The `match` method is the safest and most expressive way to handle a `Result`, as it forces the compiler to ensure you have handled both `ok` and `err` states.

```cpp
auto result = read_file("config.json");
result.match(
    // Lambda for the 'ok' case
    [](const std::string& data) {
        std::cout << "Read " << data.length() << " bytes." << std::endl;
    },
    // Lambda for the 'err' case
    [](const Error& err) {
        std::cerr << "Error: " << err.message << " (code " << err.code << ")\n";
    }
);
```

For `Result<void>`, the `ok` lambda takes no arguments:

```cpp
auto void_result = Result ::err("Disk is full");
void_result.
    match(
        [](){
            std::cout << "Operation succeeded.\n";
        },
        [](const Error& err) {
            std::cerr << "Operation failed: " << err. message << "\n";
        }
    );
```

## Extracting the Value or Error

### Safe Extraction (with defaults)

These methods provide a safe way to get the value without checking `is_ok()` first.

-   **`unwrap_or(default_value)`**: Returns the contained value or a provided default.
-   **`unwrap_or_else(lambda)`**: Returns the contained value or computes a default from a lambda.

```cpp
// Provide a simple default value
int value = Result<int>::err("Not found").unwrap_or(42); // value is 42

// Compute a default value using a lambda
int computed_value = Result<int>::err("Failed").unwrap_or_else([](const Error& err) {
    std::cerr << "Error: " << err.message << std::endl;
    return -1; // Your logic here
}); // computed_value is -1
```

### Unsafe Unwrapping

You can directly unwrap a value or error, but this will throw an exception if the `Result` is in the wrong state. This is mainly useful in tests or when an error is a fatal, unrecoverable condition.

These methods throw an exception if the `Result` is not in the expected state. **Use them only when an incorrect state is a critical, unrecoverable bug.**

-   **`unwrap()`**: Returns the value. Throws if the result is an `err`.
-   **`unwrap_move()`**: Moves and returns the value. Throws if the result is an `err`.
-   **`unwrap_err()`**: Returns the error. Throws if the result is `ok`.

```cpp
auto result = read_file("config.json");
if (result.is_ok()) {
    std::string contents = result.unwrap();
} else {
    const Error& err = result.unwrap_err();
}
```
```cpp
// In a test, where you know the result should be 'ok'.
// This will throw if Result<int>::ok(5) were an error, failing the test.
ASSERT_EQ(Result<int>::ok(5).unwrap(), 5);

// In a critical path where failure is unrecoverable.
// The program will crash with a clear message if the resource fails to load.
auto vital_resource = load_resource().unwrap(); 
```


---

### Conditional Extraction

These methods convert to other types that represent optionality, which is useful when interacting with other APIs.

-   **`to_optional()`**: Converts `Result<T>` to `std::optional<T>`. An `err` becomes `std::nullopt`.
-   **`try_unwrap()`**: Returns a `std::optional<std::reference_wrapper<T>>` (or `bool` for `Result<void>`). Useful for getting a reference without a copy.

```cpp
// Interacting with an API that uses std::optional
std::optional<int> opt = Result<int>::ok(42).to_optional(); // Contains optional with 42

// Get a reference to avoid copying a large object
auto res = Result<std::string>::ok("a very long string that we want to avoid copying");
if (auto ref_opt = res.try_unwrap()) {
    // ref_opt is std::optional<std::reference_wrapper<std::string>>
    const std::string& s_ref = ref_opt->get();
    std::cout << "Got reference to string of length: " << s_ref.length() << std::endl;
}
```

---

## Functional Composition (Chaining)

These helpers allow you to build clean, readable data processing pipelines.

-   **`map(lambda)`**: Transforms the value inside a `Result` if it is `ok`.
-   **`map_err(lambda)`**: Transforms the error inside a `Result` if it is `err`.
-   **`and_then(lambda)`**: Chains another operation that returns a `Result`.
-   **`or_else(lambda)`**: Provides a fallback operation that returns a `Result`.

### `map` and `map_err`

`map` transforms the success value while leaving an error untouched. `map_err` does the opposite.

```cpp
// Get the length of the file content
Result<size_t> length_result = read_file("config.json")
    .map([](const std::string& content) {
        return content.size();
    });

// Add context to an error message
Result<std::string> descriptive_error = read_file("nonexistent.txt")
    .map_err([](const Error& err) {
        return Error{"Config Error: " + err.message, err.code};
    });
```

### `and_then` and `or_else`

`and_then` chains operations that each return a `Result`, propagating the first error. `or_else` provides a fallback operation that can recover from an error.

```cpp
// Chain reading and parsing
Result<Config> config = read_file("config.json")
    .and_then(parse_config); // parse_config returns Result<Config>

// Try reading a primary file, then a backup
Result<std::string> content = read_file("primary.conf")
    .or_else([](const Error& err) {
        std::cout << "Primary failed, trying backup..." << std::endl;
        return read_file("backup.conf");
    });
```

```cpp
// Example: Read a file, parse it as a number, and double it.
Result<int> final_result = read_file("number.txt")
    .and_then([](const std::string& s) {
        // The parse function also returns a Result
        return parse_integer(s);
    })
    .map([](int n) {
        // This only runs if read_file and parse_integer succeeded
        return n * 2;
    })
    .map_err([](const Error& err) {
        // Add context to any error that occurred along the way
        return Error{"Calculation failed: " + err.message, err.code};
    });
```

---

## Testing and Assertions

These methods are useful for checks and tests.

-   **`contains(value)`**: Checks if the result is `ok` and holds a specific value.
-   **`expect_err(message)`**: Asserts the result is an `err`, throwing a custom message if it's `ok`.

### `contains`

Returns `true` if the result is ok and the stored value equals value.

```cpp
// Checking a specific outcome
Result<int> r = Result<int>::ok(42);
if (r.contains(42)) {
    // This code will run
}

// Forcing an error path in tests
Result<void> ok_res = Result<void>::ok();
try {
    ok_res.expect_err("This should have been an error!");
} catch (const std::runtime_error& e) {
    std::cout << e.what() << std::endl; // Prints the custom message
}
```

## ## N-API Integration Example

`Result` is ideal for bridging C++ and JavaScript, as it makes error propagation explicit.

```cpp
Napi::Value ReadFileAndProcess(const Napi::CallbackInfo& info) {
    auto result = read_file(info[0].As<Napi::String>())
        .and_then(process_data); // process_data returns Result<Processed>

    return result.match(
        [&](const Processed& data) {
            // On success, convert to a JavaScript value
            return ConvertToNapiValue(info.Env(), data);
        },
        [&](const Error& err) {
            // On error, throw a JavaScript exception
            Napi::Error::New(info.Env(), err.message).ThrowAsJavaScriptException();
            return info.Env().Undefined();
        }
    );
}
```

---

For a high-level overview, see [Result.md](Result.md).