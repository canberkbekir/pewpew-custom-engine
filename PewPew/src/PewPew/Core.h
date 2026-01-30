#pragma once

#include  <memory>

#ifdef PEW_DEBUG
#define PEW_ENABLE_ASSERTS
#endif

#ifdef PEW_ENABLE_ASSERTS
#define PEW_ASSERT(x, ...) { if(!(x)) { PEW_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#define PEW_CORE_ASSERT(x, ...) { if(!(x)) { PEW_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define PEW_ASSERT(x, ...)
	#define PEW_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)

#define PEW_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace PewPew
{
    template <typename T>
    using Scope = std::unique_ptr<T>;

    template <typename T, typename... Args>
    constexpr Scope<T> CreateScope(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template <typename T>
    using Ref = std::shared_ptr<T>;

    template <typename T, typename... Args>
    constexpr Ref<T> CreateRef(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
}
