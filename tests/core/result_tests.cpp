#include <gtest/gtest.h>
#include <NeonFS/core/result.hpp>
#include <cassert>
#include <iostream>
#include <string>

using namespace neonfs;

TEST(ResultTests, IntResultSuccess) {
    auto int_result = Result<int>::ok(42);
    EXPECT_TRUE(int_result.is_ok());
    EXPECT_FALSE(int_result.is_err());
    EXPECT_EQ(int_result.unwrap(), 42);
}

TEST(ResultTests, IntResultError) {
    auto int_error = Result<int>::err("Calculation failed", 1);
    EXPECT_TRUE(int_error.is_err());
    EXPECT_FALSE(int_error.is_ok());
    EXPECT_EQ(int_error.unwrap_err().message, "Calculation failed");
    EXPECT_EQ(int_error.unwrap_err().code, 1);
}

TEST(ResultTests, VoidResultSuccess) {
    auto void_result = Result<void>::ok();
    EXPECT_TRUE(void_result.is_ok());

    EXPECT_NO_THROW({
        void_result.unwrap();  // Should not throw
    });
}

TEST(ResultTests, VoidResultError) {
    auto void_error = Result<void>::err("Operation failed");
    EXPECT_TRUE(void_error.is_err());
    EXPECT_EQ(void_error.unwrap_err().message, "Operation failed");
}
TEST(ResultTests, UnwrapVariantsSuccess) {
    auto ok_result = Result<std::string>::ok("success");

    EXPECT_EQ(ok_result.unwrap(), "success");

    auto opt_ref = ok_result.try_unwrap();
    ASSERT_TRUE(opt_ref.has_value());
    EXPECT_EQ(opt_ref->get(), "success");

    EXPECT_EQ(ok_result.unwrap_move(), "success");
}

TEST(ResultTests, UnwrapVariantsError) {
    auto err_result = Result<std::string>::err("failed");

    EXPECT_THROW({
        err_result.unwrap();
    }, std::runtime_error);

    auto opt = err_result.try_unwrap();
    EXPECT_FALSE(opt.has_value());
}

TEST(ResultTests, MapOperationsSuccess) {
    auto mapped_ok = Result<int>::ok(2)
        .map([](int x) { return x * 3; })
        .map([](int x) { return std::to_string(x); });

    EXPECT_EQ(mapped_ok.unwrap(), "6");
}

TEST(ResultTests, MapOperationsError) {
    auto mapped_err = Result<int>::err("original error")
        .map([](int x) { return x * 2; });

    EXPECT_EQ(mapped_err.unwrap_err().message, "original error");
}

TEST(ResultTests, AndThenSuccessChain) {
    auto chained_ok = Result<int>::ok(2)
        .and_then([](int x) { return Result<int>::ok(x * 3); })
        .and_then([](int x) { return Result<std::string>::ok(std::to_string(x)); });

    EXPECT_EQ(chained_ok.unwrap(), "6");
}

TEST(ResultTests, AndThenErrorPropagation) {
    auto chained_err = Result<int>::ok(2)
        .and_then([](int) { return Result<int>::err("chain failed"); })
        .and_then([](int x) { return Result<int>::ok(x * 2); });

    EXPECT_TRUE(chained_err.is_err());
    EXPECT_EQ(chained_err.unwrap_err().message, "chain failed");
}

TEST(ResultTests, MapErrTransformsError) {
    auto mapped_error = Result<int>::err("original", 1)
        .map_err([](const Error& err) {
            return Error{err.message + " (mapped)", err.code + 1};
        });

    EXPECT_EQ(mapped_error.unwrap_err().message, "original (mapped)");
    EXPECT_EQ(mapped_error.unwrap_err().code, 2);
}

TEST(ResultTests, OrElseRecoversFromError) {
    auto recovered = Result<int>::err("temporary failure")
        .or_else([](const Error&) { return Result<int>::ok(42); });

    EXPECT_EQ(recovered.unwrap(), 42);
}

TEST(ResultTests, VoidResultChaining) {
    auto void_ok = Result<void>::ok();

    // Chain producing Result<int> from void, then map returning void
    void_ok.and_then([]() {
        return Result<int>::ok(42);
    }).map([](int x) {
        EXPECT_EQ(x, 42);
        // Explicitly returning void
    });

    // Explicit void return type in lambda
    void_ok.and_then([]() {
        return Result<int>::ok(42);
    }).map([](int x) -> void {
        EXPECT_EQ(x, 42);
    });
}

TEST(ResultTests, VoidResultErrorRecovery) {
    auto void_err = Result<void>::err("void error")
        .or_else([](const Error& err) {
            EXPECT_EQ(err.message, "void error");
            return Result<void>::ok();
        });

    EXPECT_TRUE(void_err.is_ok());
}

TEST(ResultTests, ContainsMethod) {
    auto result = Result<int>::ok(42);
    EXPECT_TRUE(result.contains(42));
    EXPECT_FALSE(result.contains(0));
}

TEST(ResultTests, ToOptionalMethod) {
    auto result = Result<int>::ok(42);
    auto opt = result.to_optional();
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt.value(), 42);

    auto empty_opt = Result<int>::err("error").to_optional();
    EXPECT_FALSE(empty_opt.has_value());
}


TEST(ResultTests, ExpectErrThrowsOnOk) {
    EXPECT_THROW({
        auto a = Result<int>::ok(1).expect_err("should fail");
    }, std::runtime_error);

    try {
        auto a = Result<int>::ok(1).expect_err("should fail");
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "should fail");
    } catch (...) {
        FAIL() << "Expected std::runtime_error";
    }
}

TEST(ResultTests, MatchWithValue) {
    auto matched = Result<int>::ok(42)
        .match(
            [](int x) { return x * 2; },
            [](const Error&) { return 0; }
        );
    EXPECT_EQ(matched, 84);
}

TEST(ResultTests, MatchWithError) {
    auto matched_err = Result<int>::err("error")
        .match(
            [](int) { return static_cast<size_t>(0); },
            [](const Error& err) { return err.message.length(); }
        );
    EXPECT_EQ(matched_err, 5);  // "error" length is 5
}

TEST(ResultTests, VoidMatch) {
    bool success_called = false;

    Result<void>::ok().match(
        [&success_called]() { success_called = true; },
        [](const Error&) { FAIL() << "Error handler should not be called"; }
    );

    EXPECT_TRUE(success_called);
}