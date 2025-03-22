#ifndef ULIGHT_RELEASABLE_VECTOR_HPP
#define ULIGHT_RELEASABLE_VECTOR_HPP

#include <cstddef>
#include <memory_resource>

#include "ulight/ulight.hpp"

#include "ulight/impl/assert.hpp"

namespace ulight {

/// A `std::vector`-like container.
/// The reason why ulight defines its own is because `std::vector` has no `.release()`
/// member function.
/// Ultimately, we need to be able to hand ownership over the allocated data to the C API,
/// and this requires extracting the pointer out of the container.
template <typename T>
struct Releasable_Vector {
public:
    using value_type = Token;
    using difference_type = std::ptrdiff_t;

private:
    std::pmr::memory_resource* m_memory;
    void* m_data = nullptr;
    size_t m_size = 0;
    size_t m_capacity = 0;

public:
    [[nodiscard]]
    explicit Releasable_Vector(std::pmr::memory_resource* memory)
        : m_memory { memory }
    {
        ULIGHT_ASSERT(memory);
    }

    [[nodiscard]]
    explicit Releasable_Vector(
        void* data,
        std::size_t size,
        std::size_t capacity,
        std::pmr::memory_resource* memory
    )
        : m_memory { memory }
        , m_data { data }
        , m_size { size }
        , m_capacity { capacity }
    {
        ULIGHT_ASSERT(memory);
    }

    [[nodiscard]]
    Releasable_Vector(const Releasable_Vector& other)
        requires std::is_copy_constructible_v<value_type>
        : m_memory { other.m_memory }
        , m_size(other.m_size)
    {
        grow_to(other.m_capacity);
        for (std::size_t i = 0; i < m_size; ++i) {
            std::construct_at(data() + i, other[i]);
        }
    }

    [[nodiscard]]
    Releasable_Vector(Releasable_Vector&& other) noexcept
        : Releasable_Vector(other.m_memory)
    {
        swap(other);
        other.clear();
    }

    ~Releasable_Vector()
    {
        clear();
        if (m_data) {
            m_memory->deallocate(m_data, m_size * sizeof(value_type), alignof(value_type));
        }
    }

    Releasable_Vector& operator=(const Releasable_Vector& other)
    {
        if (this != &other) {
            *this = auto(other);
        }
        return *this;
    }

    Releasable_Vector& operator=(Releasable_Vector&& other) noexcept
    {
        swap(other);
        other.clear();
        return *this;
    }

    [[nodiscard]]
    std::size_t size() const noexcept
    {
        return m_size;
    }

    [[nodiscard]]
    bool empty() const noexcept
    {
        return m_size == 0;
    }

    void swap(Releasable_Vector& other) noexcept
    {
        std::swap(m_memory, other.m_memory);
        std::swap(m_data, other.m_data);
        std::swap(m_size, other.m_size);
        std::swap(m_capacity, other.m_capacity);
    }

    /// Releases ownership of the data in the vector.
    [[nodiscard]]
    value_type* release() noexcept
    {
        auto* const result = data();
        m_data = nullptr;
        m_size = 0;
        m_capacity = 0;
        return result;
    }

    void clear() noexcept
    {
        for (std::size_t i = 0; i < m_size; ++i) {
            std::destroy_at(data() + i);
        }
        m_size = 0;
    }

    void resize(std::size_t size)
        requires std::is_default_constructible_v<value_type>
    {
        if (size <= m_size) {
            for (std::size_t i = size; i < m_size; ++i) {
                std::destroy_at((*this)[i]);
            }
            m_size = size;
        }
        else {
            grow_to(size);
            for (std::size_t i = m_size; i < size; ++i) {
                std::construct_at((*this)[i]);
            }
            m_size = size;
        }
    }

    void push_back(const value_type& e)
        requires std::is_copy_constructible_v<value_type>
    {
        emplace_back(e);
    }

    void push_back(value_type&& e)
        requires std::is_move_constructible_v<value_type>
    {
        emplace_back(std::move(e));
    }

    template <typename... Args>
    value_type& emplace_back(Args&&... args)
    {
        if (m_size == m_capacity) {
            grow_and_copy();
        }
        value_type& result = *std::construct_at(data() + m_size, std::forward<Args>(args)...);
        ++m_size;
        return result;
    }

    [[nodiscard]]
    value_type& front()
    {
        return (*this)[0];
    }

    [[nodiscard]]
    const value_type& front() const
    {
        return (*this)[0];
    }

    [[nodiscard]]
    value_type& back()
    {
        return (*this)[m_size - 1];
    }

    [[nodiscard]]
    const value_type& back() const
    {
        return (*this)[m_size - 1];
    }

    [[nodiscard]]
    value_type& operator[](std::size_t i)
    {
        ULIGHT_DEBUG_ASSERT(i < m_size);
        return data()[i];
    }

    [[nodiscard]]
    const value_type& operator[](std::size_t i) const
    {
        ULIGHT_DEBUG_ASSERT(i < m_size);
        return data()[i];
    }

    [[nodiscard]]
    value_type* data() noexcept
    {
        return m_data ? std::launder(static_cast<value_type*>(m_data)) : nullptr;
    }

    [[nodiscard]]
    const value_type* data() const noexcept
    {
        return m_data ? std::launder(static_cast<const value_type*>(m_data)) : nullptr;
    }

    [[nodiscard]]
    value_type* begin() noexcept
    {
        return data();
    }

    [[nodiscard]]
    const value_type* begin() const noexcept
    {
        return data();
    }

    [[nodiscard]]
    const value_type* cbegin() const noexcept
    {
        return begin();
    }

    [[nodiscard]]
    value_type* end() noexcept
    {
        return data() + m_size;
    }

    [[nodiscard]]
    const value_type* end() const noexcept
    {
        return data() + m_size;
    }

    [[nodiscard]]
    const value_type* cend() const noexcept
    {
        return end();
    }

private:
    void grow_and_copy()
    {
        const std::size_t new_capacity = m_capacity < 16 ? 16 : m_capacity * 2;
        grow_to(new_capacity);
    }

    void grow_to(std::size_t new_capacity)
    {
        if (new_capacity <= m_capacity) {
            return;
        }
        void* const new_data
            = m_memory->allocate(sizeof(value_type) * m_capacity, alignof(value_type));

        try {
            if (m_data) {
                const auto* const from = data();
                auto* const to = std::launder(static_cast<value_type*>(new_data));
                for (std::size_t i = 0; i < m_size; ++i) {
                    std::construct_at(to + i, from[i]);
                }
            }
        } catch (...) {
            m_memory->deallocate(new_data, new_capacity, alignof(value_type));
        }
        m_data = new_data;
        m_capacity = new_capacity;
    }
};

} // namespace ulight

#endif
