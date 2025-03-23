#ifndef ULIGHT_BUFFER_HPP
#define ULIGHT_BUFFER_HPP

#include <span>

#include "ulight/function_ref.hpp"

#include "ulight/impl/assert.hpp"

namespace ulight {

template <typename T>
struct Non_Owning_Buffer {
    using value_type = T;

private:
    value_type* m_buffer;
    std::size_t m_capacity;
    void* m_flush_data;
    void (*m_flush)(void*, value_type*, std::size_t);
    std::size_t m_size = 0;

public:
    [[nodiscard]]
    constexpr Non_Owning_Buffer(
        value_type* buffer,
        std::size_t capacity,
        void* flush_data,
        void (*flush)(void*, value_type*, std::size_t)
    )
        : m_buffer { buffer }
        , m_capacity { capacity }
        , m_flush_data { flush_data }
        , m_flush { flush }
    {
    }

    [[nodiscard]]
    constexpr Non_Owning_Buffer(
        std::span<value_type> buffer,
        Function_Ref<void(value_type*, std::size_t)> flush
    )
        : Non_Owning_Buffer { buffer.data(), buffer.size(), flush.get_entity(),
                              flush.get_invoker() }
    {
    }

    /// Returns the amount of elements that can be appended to the buffer before flushing.
    [[nodiscard]]
    std::size_t capacity() const noexcept
    {
        return m_capacity;
    }

    /// Returns the number of elements currently in the buffer.
    /// `size() <= capacity()` is always `true`.
    [[nodiscard]]
    std::size_t size() const noexcept
    {
        return m_size;
    }

    /// Equivalent to `m_size == 0`.
    [[nodiscard]]
    bool empty() const noexcept
    {
        return m_size == 0;
    }

    /// Sets the size to zero.
    /// Since the buffer is not responsible for the lifetimes of the elements in the buffer,
    /// none are destroyed.
    void clear() noexcept
    {
        m_size = 0;
    }

    constexpr value_type& push_back(const value_type& e)
        requires std::is_copy_assignable_v<value_type>
    {
        flush_if_lacks_space_for(1);
        return m_buffer[m_size++] = e;
    }

    constexpr value_type& push_back(value_type&& e)
        requires std::is_move_assignable_v<value_type>
    {
        flush_if_lacks_space_for(1);
        return m_buffer[m_size++] = std::move(e);
    }

    template <typename... Args>
    constexpr value_type& emplace_back(Args&&... args)
        requires std::is_constructible_v<value_type, Args&&...>
        && std::is_move_assignable_v<value_type>
    {
        flush_if_lacks_space_for(1);
        return m_buffer[m_size++] = value_type(std::forward<Args>(args)...);
    }

    [[nodiscard]]
    value_type& back()
    {
        ULIGHT_DEBUG_ASSERT(!empty());
        return m_buffer[m_size - 1];
    }

    [[nodiscard]]
    const value_type& back() const
    {
        ULIGHT_DEBUG_ASSERT(!empty());
        return m_buffer[m_size - 1];
    }

    void flush()
    {
        m_flush(m_flush_data, m_buffer, m_size);
        m_size = 0;
    }

private:
    void flush_if_lacks_space_for(std::size_t amount)
    {
        ULIGHT_DEBUG_ASSERT(m_capacity != 0);
        if (m_size + amount > m_capacity) {
            flush();
        }
    }
};

} // namespace ulight

#endif
