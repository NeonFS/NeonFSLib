#include <NeonFS/core/result.hpp>
#include <cassert>
#include <iostream>
#include <string>

using namespace neonfs;

void test_basic_operations() {
    // Test successful int result
    auto int_result = Result<int>::ok(42);
    assert(int_result.is_ok());
    assert(!int_result.is_err());
    assert(int_result.unwrap() == 42);

    // Test error int result
    auto int_error = Result<int>::err("Calculation failed", 1);
    assert(int_error.is_err());
    assert(!int_error.is_ok());
    assert(int_error.unwrap_err().message == "Calculation failed");
    assert(int_error.unwrap_err().code == 1);

    // Test void result
    auto void_result = Result<void>::ok();
    assert(void_result.is_ok());
    void_result.unwrap();  // Should not throw

    auto void_error = Result<void>::err("Operation failed");
    assert(void_error.is_err());

    std::cout << "Basic operations test passed!\n";
}

void test_unwrap_variants() {
    // Test unwrap variants
    auto ok_result = Result<std::string>::ok("success");
    assert(ok_result.unwrap() == "success");
    assert(ok_result.try_unwrap().value().get() == "success");
    assert(ok_result.unwrap_move() == "success");

    auto err_result = Result<std::string>::err("failed");
    try {
        err_result.unwrap();
        assert(false);  // Should not reach here
    } catch (const std::runtime_error& e) {
        assert(std::string(e.what()).find("failed") != std::string::npos);
    }

    assert(!err_result.try_unwrap().has_value());

    std::cout << "Unwrap variants test passed!\n";
}

void test_map_operations() {
    // Test map on success
    auto mapped_ok = Result<int>::ok(2)
        .map([](int x) { return x * 3; })
        .map([](int x) { return std::to_string(x); });

    assert(mapped_ok.unwrap() == "6");

    // Test map on error (should not apply)
    auto mapped_err = Result<int>::err("original error")
        .map([](int x) { return x * 2; });

    assert(mapped_err.unwrap_err().message == "original error");

    std::cout << "Map operations test passed!\n";
}

void test_and_then() {
    // Test successful chaining
    auto chained_ok = Result<int>::ok(2)
        .and_then([](int x) { return Result<int>::ok(x * 3); })
        .and_then([](int x) { return Result<std::string>::ok(std::to_string(x)); });

    assert(chained_ok.unwrap() == "6");

    // Test error propagation
    auto chained_err = Result<int>::ok(2)
        .and_then([](int) { return Result<int>::err("chain failed"); })
        .and_then([](int x) { return Result<int>::ok(x * 2); });

    assert(chained_err.is_err());
    assert(chained_err.unwrap_err().message == "chain failed");

    std::cout << "And_then operations test passed!\n";
}

void test_error_handling() {
    // Test map_err
    auto mapped_error = Result<int>::err("original", 1)
        .map_err([](const Error& err) {
            return Error{err.message + " (mapped)", err.code + 1};
        });

    assert(mapped_error.unwrap_err().message == "original (mapped)");
    assert(mapped_error.unwrap_err().code == 2);

    // Test or_else
    auto recovered = Result<int>::err("temporary failure")
        .or_else([](const Error&) { return Result<int>::ok(42); });

    assert(recovered.unwrap() == 42);

    std::cout << "Error handling test passed!\n";
}

void test_void_operations() {
    // Test void result operations
    auto void_ok = Result<void>::ok();

    // Chain that produces a void result
    void_ok.and_then([]() {
        return Result<int>::ok(42);
    }).map([](int x) {
        assert(x == 42);
        // Explicitly return void
        return;
    });

    // Alternative version that's more explicit
    void_ok.and_then([]() {
        return Result<int>::ok(42);
    }).map([](int x) -> void {
        assert(x == 42);
    });

    auto void_err = Result<void>::err("void error")
        .or_else([](const Error& err) {
            assert(err.message == "void error");
            return Result<void>::ok();
        });

    assert(void_err.is_ok());

    std::cout << "Void operations test passed!\n";
}

void test_utility_methods() {
    // Test contains
    auto result = Result<int>::ok(42);
    assert(result.contains(42));
    assert(!result.contains(0));

    // Test to_optional
    auto opt = result.to_optional();
    assert(opt.has_value() && opt.value() == 42);

    auto empty_opt = Result<int>::err("error").to_optional();
    assert(!empty_opt.has_value());

    // Test expect_err
    try {
        auto a = Result<int>::ok(1).expect_err("should fail");
        assert(false);
    } catch (const std::runtime_error& e) {
        assert(std::string(e.what()) == "should fail");
    }

    std::cout << "Utility methods test passed!\n";
}

void test_match() {
    // Test match with value
    auto matched = Result<int>::ok(42)
        .match(
            [](int x) { return x * 2; },
            [](const Error&) { return 0; }
        );
    assert(matched == 84);

    // Test match with error
    auto matched_err = Result<int>::err("error")
        .match(
            [](int) { return static_cast<size_t>(0); },
            [](const Error& err) { return err.message.length(); }
        );
    assert(matched_err == 5);  // "error" is 5 chars

    // Test void match
    Result<void>::ok().match(
        []() { std::cout << "Void match success\n"; },
        [](const Error&) { assert(false); }
    );

    std::cout << "Match operations test passed!\n";
}

int main() {
    test_basic_operations();
    test_unwrap_variants();
    test_map_operations();
    test_and_then();
    test_error_handling();
    test_void_operations();
    test_utility_methods();
    test_match();

    std::cout << "All Result tests passed successfully!\n";
    return 0;
}