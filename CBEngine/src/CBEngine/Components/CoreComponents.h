#pragma once

#include "CBEngine/Core/UUID.h"
#include "CBEngine/Core/String.h"

namespace CB
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