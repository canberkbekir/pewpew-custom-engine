#pragma once

#include  <memory>

// DLL export/import macros (no-ops for StaticLib builds)
#ifdef CB_BUILD_DLL
	#define CB_API __declspec(dllexport)
#elif defined(CB_IMPORT_DLL)
	#define CB_API __declspec(dllimport)
#else
	#define CB_API
#endif

#ifdef CB_DEBUG
#define CB_ENABLE_ASSERTS
#endif

#ifdef CB_ENABLE_ASSERTS
#define CB_ASSERT(x, ...) { if(!(x)) { CB_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#define CB_CORE_ASSERT(x, ...) { if(!(x)) { CB_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define CB_ASSERT(x, ...)
	#define CB_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)

#define CB_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace CB
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
