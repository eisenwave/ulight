#ifndef ULIGHT_ALGORITHM_ALL_OF_HPP
#define ULIGHT_ALGORITHM_ALL_OF_HPP

namespace ulight {

template <typename R, typename Predicate>
[[nodiscard]]
constexpr bool all_of(R&& r, Predicate predicate) // NOLINT(cppcoreguidelines-missing-std-forward)
{
    for (const auto& e : static_cast<R&&>(r)) { // NOLINT(readability-use-anyofallof)
        if (!predicate(e)) {
            return false;
        }
    }
    return true;
}

} // namespace ulight

#endif
