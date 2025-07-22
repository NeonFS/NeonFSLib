# Usage of `Result<T>`

This document explains how to effectively use the `Result<T>` type.

---

## Basic Usage

### Returning a Result

When writing functions that can fail, return a `Result<T>` instead of raw values or error codes.

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

### Handling a Result

Check the returned value and handle errors explicitly:

```cpp
auto result = read_file("config.json");

if (result.is_err()) {
     std::cerr << "Error reading file: " << result.unwrap_err().message
              << " (code " << result.unwrap_err().code << ")\n";
} else {
    std::string contents = result.unwrap();
    // Process contents...
}
```

---

## Using `Result<void>`

For functions that do not return a value but can fail:

```cpp
Result<void> write_file(const std::string& path, const std::string& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return Result<void>::err("Failed to open file for writing", errno);
    }
    file << data;
    if (!file.good()) {
        return Result<void>::err("Write failed", errno);
    }
    return Result<void>::ok();
}
```

Usage:

```cpp
auto res = write_file("output.txt", "Hello NeonFS");
if (res.is_err()) {
    std::cerr << "Write error: " << res.unwrap_err().message << std::endl;
}
```

---

## Functional Helpers

### `and_then`

Chain operations that return `Result` to propagate errors:

```cpp
Result<std::string> parse_file(const std::string& path);

auto result = read_file(path).and_then(parse_file);

if (result.is_ok()) {
    auto parsed = result.unwrap();
    // use parsed result
}
```

### `map`

Transform the successful result value:

```cpp
auto length_result = read_file(path).map([](const std::string& content) {
    return content.size();
});
```

### `or_else`

Handle errors by providing a fallback:

```cpp
auto fallback_result = read_file(path).or_else([](const Error& err) {
    std::cerr << "Error: " << err.message << ". Using fallback data.\n";
    return Result<std::string>::ok("default data");
});
```

### `expect_err`

Unwraps an error, throwing an exception with a custom message if the result is `ok`.

```cpp
Result<void> res = Result<void>::ok();
try {
    res.expect_err("Expected an error here!");
} catch (const std::runtime_error& e) {
    std::cout << e.what() << std::endl; // "Expected an error here!"
}
```

### `contains`

Returns `true` if the result is ok and the stored value equals value.

```cpp
Result<int> r = Result<int>::ok(42);
if (r.contains(42)) {
    // Yes, the value matches.
}
```

Useful for direct comparisons without calling `unwrap()`.

### `to_optional`

Converts the `Result<T>` to `std::optional<T>`.

```cpp
std::optional<int> opt = r.to_optional();
```

If `ok`, returns the value inside an `optional`.

If `err`, returns `std::nullopt`.

This is especially useful when working with APIs that expect optional.

---

## Integration with Electron Native Addon

In the N-API addon, We can use `Result<T>` to detect and propagate errors safely:

```cpp
Napi::Value ReadFile(const Napi::CallbackInfo& info) {
    auto result = FileSystem::read_file(info[0].As<Napi::String>());
    if (result.is_err()) {
        const auto& err = result.unwrap_err();
        Napi::Error::New(info.Env(), err.message).ThrowAsJavaScriptException();
        return info.Env().Undefined();
    }
    auto& data = result.unwrap();
    return Napi::Buffer<char>::Copy(info.Env(), data.data(), data.size());
}
```

---

## Tips

* Always check for `is_err()` before unwrapping.
* Use `err("message", errno)` for automatic system code capture.
* Use `try_unwrap()` if you want an optional-like safe extraction.
* Use `map`, `and_then`, and `or_else` to keep code clean and functional.
* Document returned `Error.code` meanings for clarity.
* Use `Result<void>` for operations with no meaningful return value.

---

## Summary

`Result<T>` enforces explicit, safe, and expressive error handling, improving code clarity and robustness especially when bridging C++ native code with Electron’s JavaScript environment.

---

For detailed API, see [Result.md](Result.md).
