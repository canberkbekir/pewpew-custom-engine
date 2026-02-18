#pragma once

#include "CBEngine/Core/TimeStep.h"

namespace CB
{
	class Scene;

	class ISystem
	{
	public:
		virtual ~ISystem() = default;
		virtual void Init(Scene* scene) {}
		virtual void Shutdown() {}
		virtual void OnUpdate(Scene* scene, Timestep ts) = 0;
		virtual const char* GetName() const = 0;
		virtual int GetPriority() const = 0; // lower = runs first
	};
}
