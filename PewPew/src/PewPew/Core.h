#pragma once

#ifdef PEW_PLATFORM_WINDOWS
#ifdef PEW_BUILD_DLL
#define PEW_API __declspec(dllexport)
#else
		#define PEW_API __declspec(dllimport)
#endif
#else
	#define PEW_API
#endif

#ifdef PEW_ENABLE_ASSERTS
	#define PEW_ASSERT(x, ...) { if(!(x)) { PEW_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define PEW_CORE_ASSERT(x, ...) { if(!(x)) { PEW_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
#define PEW_ASSERT(x, ...)
#define PEW_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << (x))
