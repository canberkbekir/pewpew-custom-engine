#pragma once

#include "PewPew/Core/UUID.h"
#include "PewPew/Core/String.h"

namespace PewPew
{

	struct IDComponent
	{
		UUID ID;

		IDComponent() = default;
		IDComponent(const IDComponent&) = default;
		IDComponent(UUID id) : ID(id)
		{
		}
	};

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const String& tag) : Tag(tag)
		{
		}
	};

}