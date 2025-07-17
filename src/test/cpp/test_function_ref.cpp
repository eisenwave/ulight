#include <gtest/gtest.h>

#include "ulight/function_ref.hpp"

namespace ulight {
namespace {

constexpr int sqr(int x) noexcept
{
    return x * x;
}

TEST(Function_Ref, default_construct)
{
    [[maybe_unused]]
    constexpr Function_Ref<void()> a0;
    [[maybe_unused]]
    constexpr Function_Ref<void()> a1
        = {};

    [[maybe_unused]]
    Function_Ref<void()> b;
    [[maybe_unused]]
    Function_Ref<void(int)> c;
    [[maybe_unused]]
    Function_Ref<int(void)> d;
}

TEST(Function_Ref, from_function_pointer)
{
    Function_Ref<int(int)> r0 = sqr;
    ASSERT_EQ(r0(2), 4);

    Function_Ref<int(int) const noexcept> r1 = sqr;
    ASSERT_EQ(r1(2), 4);
}

TEST(Function_Ref, from_constant)
{
    constexpr Function_Ref<int(int)> r_lambda = Constant<[](int x) { return x * x; }> {};
    static_assert(r_lambda(2) == 4);
    ASSERT_EQ(r_lambda(2), 4);

    constexpr Function_Ref<long(int)> r_lambda2 = Constant<[](int x) noexcept { return x * x; }> {};
    static_assert(r_lambda2(2) == 4);
    ASSERT_EQ(r_lambda2(2), 4);

    constexpr Function_Ref<int(int)> r_pointer = Constant<&sqr> {};
    static_assert(r_pointer(2) == 4);
    ASSERT_EQ(r_pointer(2), 4);
}

TEST(Function_Ref, from_constant_with_entity)
{
    static constexpr int s = 1;
    constexpr Function_Ref<int(int)> r_lambda
        = { Constant<[](const int* c, int x) { return (x * x) + *c; }> {}, &s };
    ASSERT_EQ(r_lambda(2), 4 + s);
}

TEST(Function_Ref, from_callable)
{
    struct Sqr {
        int operator()(int x) const
        {
            return x * x;
        }
    };

    Function_Ref<int(int)> r_dangling = Sqr {};

    Sqr sqr;
    Function_Ref<int(int)> r = sqr;
    ASSERT_EQ(r(2), 4);

    const Sqr sqr_const;
    Function_Ref<int(int)> cr0 = sqr_const;
    Function_Ref<int(int) const> cr1 = sqr_const;
    ASSERT_EQ(cr0(2), 4);
    ASSERT_EQ(cr1(2), 4);

    static constexpr Sqr sqr_constexpr;
    constexpr Function_Ref<int(int)> r_constexpr = sqr_constexpr;
    ASSERT_EQ(r_constexpr(2), 4);

    constexpr auto lambda = [](int x) { return x * x; };
    Function_Ref<int(int)> cl = lambda;
    ASSERT_EQ(cl(2), 4);

    int s = 1;
    const auto lambda_cap = [&](int x) { return (x * x) + s; };
    Function_Ref<int(int)> cl_cap0 = lambda_cap;
    Function_Ref<int(int) const> cl_cap1 = lambda_cap;
    ASSERT_EQ(cl_cap0(2), 5);
    ASSERT_EQ(cl_cap1(2), 5);
}

} // namespace
} // namespace ulight
