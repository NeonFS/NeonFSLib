#include <gtest/gtest.h>
#include <NeonFS/core/types.h>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

using namespace neonfs;

TEST(SecureAllocatorTest, SecureStringBasicUsage) {
    secure_string s = "hello";
    s += " world";
    EXPECT_EQ(s, "hello world");
}

TEST(SecureAllocatorTest, SecureBytesBasicUsage) {
    secure_bytes b = {0x01, 0x02, 0x03};
    b.push_back(0x04);
    ASSERT_EQ(b.size(), 4);
    EXPECT_EQ(b[3], 0x04);
}

TEST(SecureAllocatorTest, SecureVectorBasicUsage) {
    secure_vector<int> v = {1, 2, 3};
    v.push_back(4);
    EXPECT_EQ(v.back(), 4);
}

TEST(SecureAllocatorTest, SecureListBasicUsage) {
    secure_list<int> l = {10, 20};
    l.push_back(30);
    EXPECT_EQ(l.back(), 30);
}

TEST(SecureAllocatorTest, SecureDequeBasicUsage) {
    secure_deque<int> d = {1, 2};
    d.push_front(0);
    EXPECT_EQ(d.front(), 0);
}

#include <array>

TEST(SecureAllocatorTest, SecureUnorderedSetBasicUsage) {
    secure_unordered_set<char> s;

    s.insert('a');
    s.insert('b');
    s.insert('c');

    EXPECT_TRUE(s.find('b') != s.end());
    EXPECT_FALSE(s.find('z') != s.end());
}

TEST(SecureAllocatorTest, SecureMapBasicUsage) {
    using fixed_string = std::array<char, 16>;

    secure_map<fixed_string, int> m;

    fixed_string one = {'o','n','e','\0'};
    fixed_string two = {'t','w','o','\0'};

    m[one] = 1;
    m[two] = 2;

    EXPECT_EQ(m[one], 1);
}

TEST(SecureAllocatorTest, SecureMapIntKeyUsage) {
    secure_map<int, int> m;
    m[1] = 10;
    m[2] = 20;
    EXPECT_EQ(m[1], 10);
    EXPECT_EQ(m[2], 20);
}

TEST(SecureAllocatorTest, SecureUnorderedMapIntKeyUsage) {
    secure_unordered_map<int, int> um;
    um[1] = 10;
    um[2] = 20;
    EXPECT_EQ(um.at(2), 20);
}

TEST(SecureAllocatorTest, SecureStringMemoryIsZeroedOnDeallocate) {
    constexpr size_t secret_len = 17;
    const char* raw_ptr = nullptr;

    {
        secure_string s("SuperSecret123456");
        ASSERT_EQ(s.size(), secret_len);

        // Force non-SBO allocation by resizing (if short string optimization is on)
        s.reserve(secret_len + 1);
        s[0] = 'S'; // Make sure it's non-empty

        // Save internal pointer before destruction
        raw_ptr = const_cast<char*>(s.data());

        // Ensure data is there
        ASSERT_STREQ(raw_ptr, "SuperSecret123456");
    }

    // At this point, s is destroyed. The secure_allocator should've zeroed the memory.
    // We assume raw_ptr still points to the previously allocated buffer.

    // WARNING: This only works if your allocator uses malloc/secure_malloc
    // and doesn't overwrite raw_ptr or reuse it immediately.

    // Check if the memory is zeroed
    bool all_zero = true;
    for (size_t i = 0; i < secret_len; ++i) {
        if (raw_ptr[i] != 0) {
            all_zero = false;
            break;
        }
    }

    EXPECT_TRUE(all_zero) << "Memory was not securely wiped after deallocation";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    initialize_secure_heap();

    int result = RUN_ALL_TESTS();

    // Clean up secure heap after tests finish
    cleanup_secure_heap();

    return result;
}