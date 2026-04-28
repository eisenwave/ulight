#ifndef ULIGHT_CHARSET_HPP
#define ULIGHT_CHARSET_HPP

#include <cstdint>

#include "ulight/impl/assert.hpp"

namespace ulight {

template <std::size_t N>
struct Charset {
    static constexpr std::size_t width = N;
    static constexpr std::size_t limb_width = 64;
    static constexpr std::size_t limb_count = (N / 64) + (N % 64 != 0);
    using limb_type = std::uint64_t;

    /// Returns a `Charset` where the characters in `[i, j)` are present.
    [[nodiscard]]
    static constexpr Charset range_before(const std::size_t i, const std::size_t j)
    {
        ULIGHT_ASSERT(i < 256);
        ULIGHT_ASSERT(j <= 256);
        ULIGHT_ASSERT(i <= j);

        Charset result;
        const limb_type all_bits = ~limb_type { 0 };
        for (std::size_t limb = 0; limb < limb_count; ++limb) {
            const std::size_t begin = limb * limb_width;
            const std::size_t lo = i > begin ? i - begin : 0;
            const std::size_t hi = j < begin + limb_width ? j - begin : limb_width;
            if (lo < hi) {
                const limb_type lo_mask = all_bits << lo;
                const limb_type hi_mask = hi == limb_width ? all_bits : (limb_type { 1 } << hi) - 1;
                result.limbs[limb] = lo_mask & hi_mask;
            }
        }
        return result;
    }

    /// Returns a `Charset` where the characters in `[i, j]` are present.
    [[nodiscard]]
    static constexpr Charset range_until(const std::size_t i, const std::size_t j)
    {
        return range_before(i, j + 1);
    }

    /// Returns a `Charset` containing characters for which `predicate` returns `true`.
    [[nodiscard]]
    static consteval Charset from_predicate(bool predicate(char8_t))
    {
        static_assert(N >= 256);

        Charset result {};
        for (std::size_t i = 0; i < 256; ++i) {
            if (predicate(char8_t(i))) {
                result.insert(char8_t(i));
            }
        }
        return result;
    }

private:
    limb_type limbs[limb_count] {};

public:
    [[nodiscard]]
    Charset()
        = default;
    [[nodiscard]]
    constexpr explicit Charset(const char8_t c) noexcept
    {
        set(std::size_t(c));
    }
    [[nodiscard]]
    constexpr explicit Charset(const std::u8string_view chars) noexcept
    {
        for (const char8_t c : chars) {
            set(std::size_t(c));
        }
    }

    [[nodiscard]]
    constexpr bool operator()(const char8_t c) const noexcept(N >= 256)
    {
        return get(std::size_t(c));
    }
    [[nodiscard]]
    constexpr bool operator()(const char32_t c) const noexcept(N >= 256)
    {
        return c <= 0xff ? get(char8_t(c)) : false;
    }

    constexpr void clear() noexcept
    {
        *this = {};
    }

    constexpr void remove(char8_t c) noexcept(N >= 256)
    {
        clear(std::size_t(c));
    }

    constexpr void insert(char8_t c) noexcept(N >= 256)
    {
        set(std::size_t(c));
    }

    [[nodiscard]]
    friend constexpr bool operator==(const Charset& x, const Charset& y)
        = default;

    [[nodiscard]]
    constexpr Charset operator~() const noexcept
    {
        Charset result;
        for (std::size_t i = 0; i < limb_count; ++i) {
            result.limbs[i] = ~limbs[i];
        }
        return result;
    }

    constexpr Charset& operator|=(const Charset& other) noexcept
    {
        for (std::size_t i = 0; i < limb_count; ++i) {
            limbs[i] |= other.limbs[i];
        }
        return *this;
    }

    constexpr Charset& operator|=(char8_t c) noexcept(N >= 256)
    {
        set(std::size_t(c));
        return *this;
    }

    [[nodiscard]]
    friend constexpr Charset operator|(const Charset& x, const Charset& y) noexcept
    {
        Charset result = x;
        result |= y;
        return result;
    }

    [[nodiscard]]
    friend constexpr Charset operator|(const Charset& x, const char8_t c) noexcept
    {
        Charset result = x;
        result |= c;
        return result;
    }

    constexpr Charset& operator&=(const Charset& other) noexcept
    {
        for (std::size_t i = 0; i < limb_count; ++i) {
            limbs[i] &= other.limbs[i];
        }
        return *this;
    }

    [[nodiscard]]
    friend constexpr Charset operator&(const Charset& x, const Charset& y) noexcept
    {
        Charset result = x;
        result &= y;
        return result;
    }

    constexpr Charset& operator-=(const Charset& other) noexcept
    {
        for (std::size_t i = 0; i < limb_count; ++i) {
            limbs[i] &= ~other.limbs[i];
        }
        return *this;
    }

    constexpr Charset& operator-=(char8_t c) noexcept(N >= 256)
    {
        clear(std::size_t(c));
        return *this;
    }

    [[nodiscard]]
    friend constexpr Charset operator-(const Charset& x, const Charset& y) noexcept
    {
        Charset result = x;
        result -= y;
        return result;
    }

    [[nodiscard]]
    friend constexpr Charset operator-(const Charset& x, char8_t c) noexcept
    {
        Charset result = x;
        result -= c;
        return result;
    }

private:
    [[nodiscard]]
    constexpr bool get(std::size_t i) const
    {
        ULIGHT_DEBUG_ASSERT(i < width);
        return (limbs[i / limb_width] >> (i % limb_width)) & 1;
    }

    constexpr void clear(std::size_t i)
    {
        ULIGHT_DEBUG_ASSERT(i < width);
        limbs[i / limb_width] &= ~(std::uint64_t { 1 } << (i % limb_width));
    }

    constexpr void set(std::size_t i)
    {
        ULIGHT_DEBUG_ASSERT(i < width);
        limbs[i / limb_width] |= std::uint64_t { 1 } << (i % limb_width);
    }

    constexpr void set(std::size_t i, bool value)
    {
        ULIGHT_DEBUG_ASSERT(i < width);
        clear(i);
        limbs[i / limb_width] |= std::uint64_t { value } << (i % limb_width);
    }
};

using Charset128 = Charset<128>;
using Charset256 = Charset<256>;

} // namespace ulight

#endif
