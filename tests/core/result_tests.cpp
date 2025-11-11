#include <gtest/gtest.h>
#include <string>
#include <memory>       // For std::unique_ptr
#include <stdexcept>    // For std::runtime_error

#include <NeonFS/core/result.hpp>

using namespace neonfs;

TEST(ResultTests, BasicConstruction) {
  // Result<T>
  auto ok_t = Result<int>::ok(42);
  EXPECT_TRUE(ok_t.is_ok());
  EXPECT_FALSE(ok_t.is_err());
  EXPECT_TRUE(ok_t);

  auto err_t = Result<int>::err("fail", 1);
  EXPECT_FALSE(err_t.is_ok());
  EXPECT_TRUE(err_t.is_err());
  EXPECT_FALSE(err_t);

  // Result<void>
  auto ok_v = Result<void>::ok();
  EXPECT_TRUE(ok_v.is_ok());
  EXPECT_FALSE(ok_v.is_err());
  EXPECT_TRUE(ok_v);

  auto err_v = Result<void>::err("fail", 2);
  EXPECT_FALSE(err_v.is_ok());
  EXPECT_TRUE(err_v.is_err());
  EXPECT_FALSE(err_v);
}

TEST(ResultTests, InPlaceFactories) {
  auto ok_t = Result<std::string>::ok_in_place(5, 'a'); // string("aaaaa")
  EXPECT_TRUE(ok_t.is_ok());
  EXPECT_EQ(ok_t.unwrap(), "aaaaa");

  auto err_t = Result<int>::err_in_place("bad error", 123);
  EXPECT_TRUE(err_t.is_err());
  EXPECT_EQ(err_t.unwrap_err().message, "bad error");
  EXPECT_EQ(err_t.unwrap_err().code, 123);
}

TEST(ResultTests, UnwrapAndExpectT) {
  Result<int> ok_res(Result<int>::ok(10));
  const Result<int> const_ok_res(Result<int>::ok(10));

  // Test & and const & overloads
  EXPECT_EQ(ok_res.unwrap(), 10);
  EXPECT_EQ(const_ok_res.unwrap(), 10);
  EXPECT_NO_THROW(ok_res.expect("should be ok"));

  // Test && overload
  EXPECT_EQ(Result<int>::ok(20).unwrap(), 20);

  // Test error cases
  Result<int> err_res(Result<int>::err("fail"));
  ASSERT_THROW(err_res.unwrap(), std::runtime_error);
  ASSERT_THROW(err_res.expect("custom"), std::runtime_error);

  // Test exception message
  try {
    err_res.expect("custom");
  } catch (const std::runtime_error& e) {
    EXPECT_STREQ(e.what(), "custom: fail"); // EXPECT_THAT(e.what(), HasSubstr("custom: fail"));
  }
}

TEST(ResultTests, UnwrapAndExpectVoid) {
  Result<void> ok_res(Result<void>::ok());
  Result<void> err_res(Result<void>::err("fail"));

  EXPECT_NO_THROW(ok_res.unwrap());
  EXPECT_NO_THROW(ok_res.expect("should be ok"));

  ASSERT_THROW(err_res.unwrap(), std::runtime_error);
  ASSERT_THROW(err_res.expect("custom"), std::runtime_error);

  try {
    err_res.expect("custom");
  } catch (const std::runtime_error& e) {
    EXPECT_STREQ(e.what(), "custom: fail");
  }
}

TEST(ResultTests, UnwrapAndExpectErr) {
  Result<int> ok_res(Result<int>::ok(10));
  Result<int> err_res(Result<int>::err("fail", 99));
  const Result<int> const_err_res(Result<int>::err("const fail", 98));

  // Test & and const &
  EXPECT_EQ(err_res.unwrap_err().message, "fail");
  EXPECT_EQ(const_err_res.unwrap_err().code, 98);
  EXPECT_NO_THROW(err_res.expect_err("should be err"));

  // Test &&
  EXPECT_EQ(Result<int>::err("moved fail").unwrap_err().message, "moved fail");

  // Test error cases
  ASSERT_THROW(ok_res.unwrap_err(), std::runtime_error);
  ASSERT_THROW(ok_res.expect_err("custom"), std::runtime_error);

  // Test exception message
  try {
    ok_res.expect_err("custom");
  } catch (const std::runtime_error& e) {
    EXPECT_STREQ(e.what(), "custom");
  }
}

TEST(ResultTests, TryUnwrapAndToOptional) {
  Result<int> ok_res(Result<int>::ok(10));
  const Result<int> const_ok_res(Result<int>::ok(20));
  Result<int> err_res(Result<int>::err("fail"));

  // try_unwrap() -> T*
  int* val_ptr = ok_res.try_unwrap();
  ASSERT_NE(val_ptr, nullptr);
  EXPECT_EQ(*val_ptr, 10);
  *val_ptr = 11; // Test mutability
  EXPECT_EQ(ok_res.unwrap(), 11);

  // try_unwrap() -> const T*
  const int* const_val_ptr = const_ok_res.try_unwrap();
  ASSERT_NE(const_val_ptr, nullptr);
  EXPECT_EQ(*const_val_ptr, 20);
  // *const_val_ptr = 21; // <-- Should fail to compile (with: Cannot assign to readonly type const int), which is correct, i don't know how to test this :>

  // try_unwrap() on Err
  EXPECT_EQ(err_res.try_unwrap(), nullptr);

  // to_optional()
  EXPECT_EQ(ok_res.to_optional(), std::optional<int>(11));
  EXPECT_EQ(const_ok_res.to_optional(), std::optional<int>(20));
  EXPECT_EQ(err_res.to_optional(), std::nullopt);
  EXPECT_EQ(Result<int>::ok(50).to_optional(), std::optional<int>(50)); // Test && overload
}

TEST(ResultTests, UnwrapOr) {
  EXPECT_EQ(Result<int>::ok(10).unwrap_or(5), 10);
  EXPECT_EQ(Result<int>::err("fail").unwrap_or(5), 5);

  // Test && overload
  EXPECT_EQ(Result<int>::err("fail").unwrap_or(std::string("default").length()), 7);
}

TEST(ResultTests, UnwrapOrElse) {
  // Result<T>
  auto or_else_t = [](const Error& e) -> int { return e.message.length(); };
  EXPECT_EQ(Result<int>::ok(10).unwrap_or_else(or_else_t), 10);
  EXPECT_EQ(Result<int>::err("fail").unwrap_or_else(or_else_t), 4);

  // Result<void>
  bool called = false;
  auto or_else_v = [&](const Error& e) { called = true; };
  Result<void>::ok().unwrap_or_else(or_else_v);
  EXPECT_FALSE(called);

  Result<void>::err("fail").unwrap_or_else(or_else_v);
  EXPECT_TRUE(called);
}

TEST(ResultTests, Map) {
  auto to_string = [](int x) { return std::to_string(x); };
  auto to_void = [](int x) {};

  // T -> U (const &)
  const auto ok_t = Result<int>::ok(42);
  EXPECT_EQ(ok_t.map(to_string).unwrap(), "42");

  // T -> void
  EXPECT_TRUE(ok_t.map(to_void).is_ok());

  // Error propagation
  const auto err_t = Result<int>::err("fail");
  EXPECT_EQ(err_t.map(to_string).unwrap_err().message, "fail");

  // void -> U
  EXPECT_EQ(Result<void>::ok().map([]() { return 10; }).unwrap(), 10);

  // void -> void
  EXPECT_TRUE(Result<void>::ok().map([]() {}).is_ok());
}

TEST(ResultTests, AndThen) {
  auto to_str_res = [](int x) { return Result<std::string>::ok(std::to_string(x)); };
  auto to_err_res = [](int x) { return Result<std::string>::err("inner"); };

  // Success chain (const &)
  const auto ok_t = Result<int>::ok(42);
  EXPECT_EQ(ok_t.and_then(to_str_res).unwrap(), "42");

  // Failure chain
  EXPECT_EQ(ok_t.and_then(to_err_res).unwrap_err().message, "inner");

  // Error propagation (short-circuit)
  const auto err_t = Result<int>::err("outer");
  int calls = 0;
  auto counter_fn = [&](int x) { calls++; return Result<int>::ok(x); };
  EXPECT_EQ(err_t.and_then(counter_fn).unwrap_err().message, "outer");
  EXPECT_EQ(calls, 0); // Prove it was never called
}

TEST(ResultTests, OrElse) {
  auto recover = [](const Error& e) { return Result<int>::ok(e.message.length()); };
  auto fail_recover = [](const Error& e) { return Result<int>::err("still fail"); };

  // Ok pass-through (const &)
  const auto ok_t = Result<int>::ok(42);
  EXPECT_EQ(ok_t.or_else(recover).unwrap(), 42);

  // Error recovery
  const auto err_t = Result<int>::err("fail");
  EXPECT_EQ(err_t.or_else(recover).unwrap(), 4);

  // Chained error
  EXPECT_EQ(err_t.or_else(fail_recover).unwrap_err().message, "still fail");
}

TEST(ResultTests, MapErr) {
  auto remap = [](const Error& e) { return Error{e.message + "ed", 10}; };

  // Ok pass-through (const &)
  const auto ok_t = Result<int>::ok(42);
  EXPECT_EQ(ok_t.map_err(remap).unwrap(), 42);

  // Error remapping
  const auto err_t = Result<int>::err("fail");
  auto remap_res = err_t.map_err(remap);
  EXPECT_EQ(remap_res.unwrap_err().message, "failed");
  EXPECT_EQ(remap_res.unwrap_err().code, 10);
}

TEST(ResultTests, MoveOnlyTypeSupport) {
  auto factory = []() {
    return Result<std::unique_ptr<int>>::ok(std::make_unique<int>(10));
  };

  // Test unwrap() &&
  auto p = factory().unwrap(); // Moves the unique_ptr out
  EXPECT_EQ(*p, 10);

  // Test operator->
  auto res_ptr = factory();
  EXPECT_EQ(*(res_ptr.operator->()->get()), 10);

  // Test operator*
  EXPECT_EQ(**res_ptr, 10); // Calls ->unwrap().operator*()

  // Test map() && (move-only chain)
  auto map_res = factory().map([](std::unique_ptr<int> p_in) {
      *p_in += 5;
      return p_in; // p_in is moved out
  });
  EXPECT_TRUE(map_res.is_ok());
  EXPECT_EQ(**map_res, 15);

  // Test and_then() && (full move-only chain)
  auto final_res = factory()
      .and_then([](std::unique_ptr<int> p_in) {
          *p_in += 1; // *p_in is 11
          return Result<std::unique_ptr<std::string>>::ok(
              std::make_unique<std::string>(std::to_string(*p_in))
          );
      })
      .map([](std::unique_ptr<std::string> s_in) {
          return *s_in + "!"; // s_in is moved, string is copied
      });

  EXPECT_TRUE(final_res.is_ok());
  EXPECT_EQ(final_res.unwrap(), "11!");

  // Test move-only error propagation
  auto err_chain = factory()
      .and_then([](std::unique_ptr<int> p_in) {
          return Result<int>::err("chain fail");
      })
      .map([](int x) { return x + 1; });

  EXPECT_TRUE(err_chain.is_err());
  EXPECT_EQ(err_chain.unwrap_err().message, "chain fail");
}

TEST(ResultTests, ContainsMethod) {
  auto result = Result<int>::ok(42);
  EXPECT_TRUE(result.contains(42));
  EXPECT_FALSE(result.contains(0));
}