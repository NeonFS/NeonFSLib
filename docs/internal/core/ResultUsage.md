# Usage of `Result<T>`

This document explains how to effectively use the `Result<T>` type with practical examples.

---

## Creating a `Result`

You can create a success (`ok`) or error (`err`) result. The `ok` method allows in-place construction of the contained value, making it more convenient.

```cpp
// Create a success Result with a value
auto success = neonfs::Result<int>::ok(42);

// In-place construction for complex types
auto text_result = neonfs::Result<std::string>::ok("hello world");

// Success with no value
auto void_success = neonfs::Result<void>::ok();

// Create an error Result with a message
auto error = neonfs::Result<int>::err("File not found");
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

### Checking State and Accessing Values

The most idiomatic way to check a `Result` is to use it in a boolean context.

```cpp
auto read_file(const std::string& path) -> neonfs::Result<std::string>;

auto result = read_file("my_file.txt");

if (result) { // The idiomatic check using operator bool
    std::cout << "File content: " << result.unwrap() << std::endl;
} else {
    std::cerr << "Error: " << result.unwrap_err().message << std::endl;
}
```

### Handling Outcomes with `match` (Preferred Method)

Use `match()` to handle both success and error cases in a single, expressive statement. Both provided functions must return the same type.

```cpp
auto result = read_file("another_file.txt");

std::string message = result.match(
    [](const std::string& value) { return "File read successfully. Size: " + std::to_string(value.length()); },
    [](const neonfs::Error& err) { return "File read failed: " + err.message; }
);
std::cout << message << std::endl;
```

For `Result<void>`, the `ok` lambda takes no arguments:

```cpp
auto void_result = neonfs::Result<void>::err("Disk is full");
void_result.match(
    []() {
        std::cout << "Operation succeeded.\n";
    },
    [](const neonfs::Error& err) {
        std::cerr << "Operation failed: " << err.message << "\n";
    }
);
```

## Extracting the Value or Error

### Safe Extraction (Unwrapping with defaults)

When an operation fails, you can provide a default value or compute one.

```cpp
auto config = neonfs::Result<int>::err("Not found");

// Provide a simple default value
int port = config.unwrap_or(8080); // port is 8080

// Compute a default value using a lambda
int fallback_port = config.unwrap_or_else([](const neonfs::Error& e) {
    std::cerr << "Config error: " << e.message << ". Using fallback." << std::endl;
    return 9000; 
}); // fallback_port is 9000
```

### Pointer-like Access

For convenience, you can access the members of a success value directly using `->` and `*`. **This will throw an exception if the result is an error**.

```cpp
struct Config { std::string setting = "default"; };
auto result = neonfs::Result<Config>::ok({});

// Access member directly using ->
std::cout << "Setting: " << result->setting << std::endl;

// Dereference using *
Config& cfg_ref = *result;
std::cout << "Setting (via reference): " << cfg_ref.setting << std::endl;
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
-   **`try_unwrap()`**: Returns a `std::optional<std::reference_wrapper<T>>` (or `std::optional<std::monostate>` for `Result<void>`). Useful for getting a reference without a copy.

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

## Functional Chaining with Combinators

Combinators enable clean, functional pipelines for complex operations where any step can fail.

- **`and_then(fn)`**: If `ok`, calls `fn` with the value. `fn` must return a `Result`.
- **`or_else(fn)`**: If `err`, calls `fn` with the error. `fn` must return a `Result`, allowing recovery.
- **`map(fn)`**: If `ok`, applies a transformation to the value and wraps it in a new `Result`.
- **`map_err(fn)`**: If `err`, applies a transformation to the error.

```cpp
// Functions that can fail
auto get_config(const std::string& key) -> neonfs::Result<std::string>;
auto to_int(const std::string& s) -> neonfs::Result<int>;

// Attempt to get and parse a port number, with recovery
auto result = get_config("port") // Might return err("Not found")
    .or_else([](const neonfs::Error& e) -> neonfs::Result<std::string> {
        std::cout << "Could not get port, falling back to default '8080'\n";
        return neonfs::Result<std::string>::ok("8080"); // Recover
    })
    .and_then(to_int) // Chain the next fallible operation
    .map([](int port) { return port + 1; }); // Transform the success value

if (result) {
    std::cout << "Next available port: " << *result << std::endl;
}
```

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
Result<Config> config = read_config_file("config.json")
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

## N-API Integration Example

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