#pragma once

#include "PewPew/Core/String.h"

namespace PewPew
{
	class FileDialogs
	{
	public:
		// Returns empty string if cancelled
		static String OpenFile(const char* filter);
		static String SaveFile(const char* filter);
	};
}
