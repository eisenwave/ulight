#ifndef ULIGHT_FUNCTION_REF_HPP
#define ULIGHT_FUNCTION_REF_HPP

#include <concepts>
#include <exception>
#include <type_traits>
#include <utility>

#include "ulight/const.hpp"

#include "ulight/impl/assert.hpp"

namespace ulight {

template <typename T>
concept function_pointer = std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>;

template <typename T, typename R, typename... Args>
concept invocable_r
    = std::invocable<T, Args...> && std::convertible_to<std::invoke_result_t<T, Args...>, R>;

template <typename T, typename... Args>
concept nothrow_invocable = std::invocable<T, Args...> && std::is_nothrow_invocable_v<T, Args...>;

template <typename T, typename R, typename... Args>
concept nothrow_invocable_r = invocable_r<T, R, Args...> && nothrow_invocable<T, Args...>;

template <typename T, bool nothrow, typename R, typename... Args>
concept invocable_n_r
    = invocable_r<T, R, Args...> && (!nothrow || nothrow_invocable_r<T, R, Args...>);

template <bool constant, bool nothrow, typename R, typename... Args>
struct Function_Ref_Base {
public:
    using Function = R(Args...) noexcept(nothrow);
    using Storage = const void;
    using Invoker = R(Storage*, Args...) noexcept(nothrow);

private:
    template <typename F>
        requires std::is_pointer_v<F>
    static R call(Storage* entity, Args... args) noexcept(nothrow)
    {
        if constexpr (std::is_function_v<std::remove_pointer_t<F>>) {
            // This 'const_cast' is needed because Clang does not support conversions from
            // 'const void*' to function pointer types.
            // This could be considered a bug or a language defect
            // (see https://github.com/cplusplus/CWG/issues/657).
            // In any case, we need to remove 'const'.
            void* const entity_raw = const_cast<void*>(entity);
            return R((*reinterpret_cast<F>(entity_raw))(std::forward<Args>(args)...));
        }
        else {
            using Const_F = const std::remove_pointer_t<F>*;
            F f = const_cast<F>(static_cast<Const_F>(entity));
            return R((*f)(std::forward<Args>(args)...));
        }
    }

    Invoker* m_invoker = nullptr;
    Storage* m_entity = nullptr;

public:
    [[nodiscard]]
    constexpr Function_Ref_Base()
        = default;

    /// @brief Constructs a `Function_Ref` piecewise from its invoker and entity.
    [[nodiscard]]
    constexpr Function_Ref_Base(Invoker* invoker, Storage* entity) noexcept
        : m_invoker { invoker }
        , m_entity { entity }
    {
    }

    /// @brief Constructs a `Function_Ref` from a compile-time constant which is convertible
    /// to a function pointer.
    ///
    /// This will create a `Function_Ref` which is bound to nothing,
    /// and when called, simply forwards to `F`.
    template <invocable_n_r<nothrow, R, Args...> auto F>
    [[nodiscard]]
    constexpr Function_Ref_Base(Constant<F>) noexcept
        : m_invoker { [](Storage*, Args... args) noexcept(nothrow) -> R { //
            return F(std::forward<Args>(args)...);
        } }
    {
    }

    /// @brief Constructs a `Function_Ref` from a compile-time constant which is convertible
    /// to a function pointer.
    ///
    /// This will create a `Function_Ref` which is bound to nothing,
    /// and when called, simply forwards to `F`.
    template <auto F, typename T>
        requires invocable_n_r<decltype(F), nothrow, R, T*, Args...>
                     && std::convertible_to<T*, Storage*>
    [[nodiscard]]
    constexpr Function_Ref_Base(Constant<F>, T* entity) noexcept
        : m_invoker { [](Storage* entity, Args... args) noexcept(nothrow) { //
            return F(const_cast<T*>(static_cast<const T*>(entity)), std::forward<Args>(args)...);
        } }
        , m_entity { entity }
    {
    }

    /// @brief Constructs a `Function_Ref` from non-constant function pointer.
    ///
    /// Unlike most other constructors,
    /// this is not marked `constexpr` because it unconditionally requires the use of
    /// `reinterpret_cast`.
    [[nodiscard]]
    Function_Ref_Base(Function* f) noexcept
        : m_invoker(&call<Function*>)
        , m_entity(reinterpret_cast<Storage*>(f))
    {
    }

    /// @brief Constructs a `Function_Ref` from some callable type.
    ///
    /// If `f` is convertible to `R(*)(Args...)`,
    /// this constructor accepts both lvalues and rvalues.
    /// This applies in the case of function references, function pointers, captureless lambdas,
    /// etc.
    /// The `Function_Ref` will bind to the given function pointer in such a case.
    ///
    /// Otherwise, only lvalues are accepted, and the `Function_Ref` binds to `f`.
    template <typename F>
        requires(!std::same_as<std::remove_cvref_t<F>, Function_Ref_Base> && invocable_n_r<follow_ref_const_if_t<F, constant>, nothrow, R, Args...>)
    [[nodiscard]]
    constexpr Function_Ref_Base(F&& f) noexcept // NOLINT(cppcoreguidelines-missing-std-forward)
    {
        using Entity = std::remove_reference_t<F>;

        if constexpr (std::is_function_v<Entity>) {
            m_invoker = &call<Entity* const>;
            m_entity = reinterpret_cast<Storage*>(&f);
        }
        else if constexpr (function_pointer<Entity>) {
            m_invoker = &call<const Entity>;
            m_entity = reinterpret_cast<Storage*>(f);
        }
        else if constexpr (std::is_convertible_v<F&&, Function*>) {
            Function* const pointer = f;
            m_invoker = &call<decltype(pointer)>;
            m_entity = reinterpret_cast<Storage*>(pointer);
        }
        else if constexpr (requires {
                               { +f } -> function_pointer;
                           }) {
            // This case covers e.g. captureless lambdas.
            // Those can always be converted to function pointers, but not exactly to
            // Function_Pointer_Type; that case has already been handled above.
            auto pointer = +f;
            m_invoker = &call<decltype(pointer)>;
            m_entity = reinterpret_cast<Storage*>(pointer);
        }
        else {
            m_invoker = &call<const_if_t<Entity, constant>*>;
            m_entity = std::addressof(f);
        }
    }

    constexpr R operator()(Args... args) const noexcept(nothrow)
    {
        if constexpr (nothrow) {
            if (!m_invoker) {
                std::terminate();
            }
        }
        else {
            ULIGHT_ASSERT(m_invoker);
        }
        return m_invoker(m_entity, std::forward<Args>(args)...);
    }

    [[nodiscard]]
    constexpr bool has_value() const noexcept
    {
        return m_invoker != nullptr;
    }

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept
    {
        return has_value();
    }

    [[nodiscard]]
    Invoker* get_invoker() const noexcept
    {
        return m_invoker;
    }

    [[nodiscard]]
    Storage* get_entity() const noexcept
    {
        return m_entity;
    }
};

template <typename F>
struct Function_Ref;

template <typename R, typename... Args>
struct Function_Ref<R(Args...)> : Function_Ref_Base<false, false, R, Args...> {
    using Function_Ref_Base<false, false, R, Args...>::Function_Ref_Base;
};

template <typename R, typename... Args>
struct Function_Ref<R(Args...) noexcept> : Function_Ref_Base<false, true, R, Args...> {
    using Function_Ref_Base<false, true, R, Args...>::Function_Ref_Base;
};

template <typename R, typename... Args>
struct Function_Ref<R(Args...) const> : Function_Ref_Base<true, false, R, Args...> {
    using Function_Ref_Base<true, false, R, Args...>::Function_Ref_Base;
};

template <typename R, typename... Args>
struct Function_Ref<R(Args...) const noexcept> : Function_Ref_Base<true, true, R, Args...> {
    using Function_Ref_Base<true, true, R, Args...>::Function_Ref_Base;
};

} // namespace ulight

#endif
