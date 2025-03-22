#ifndef ULIGHT_MEMORY_HPP
#define ULIGHT_MEMORY_HPP

#include <memory_resource>

#include "ulight/ulight.hpp"

#include "ulight/impl/assert.hpp"

namespace ulight {

struct Memory_Resource final : std::pmr::memory_resource {
private:
    Alloc_Function* m_alloc;
    Free_Function* m_free;

public:
    Memory_Resource(Alloc_Function* alloc, Free_Function* free)
        : m_alloc { alloc }
        , m_free { free }
    {
        ULIGHT_ASSERT(alloc);
        ULIGHT_ASSERT(free);
    }

    explicit Memory_Resource(const State& state)
        : Memory_Resource { state.get_alloc(), state.get_free() }
    {
    }

    [[nodiscard]]
    void* do_allocate(std::size_t bytes, std::size_t alignment) final
    {
        void* result = m_alloc(bytes, alignment);
        if (!result) {
            throw std::bad_alloc();
        }
        return result;
    }

    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) noexcept final
    {
        m_free(p, bytes, alignment);
    }

    [[nodiscard]]
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept final
    {
        if (const auto* r = dynamic_cast<const Memory_Resource*>(&other)) {
            return m_alloc == r->m_alloc && m_free == r->m_free;
        }
        return false;
    }
};

} // namespace ulight

#endif
