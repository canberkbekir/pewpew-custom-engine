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

#define BIT(x) (1 << (x))