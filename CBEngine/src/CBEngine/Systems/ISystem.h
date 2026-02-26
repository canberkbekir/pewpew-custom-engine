#pragma once

#include "CBEngine/Core/TimeStep.h"

namespace CB
{
	class Scene;

	class ISystem
	{
	public:
		virtual ~ISystem() = default;
		virtual void Init(Scene* scene)
		{
		}
		virtual void Shutdown()
		{
		}
		virtual void OnUpdate(Scene* scene,Timestep ts) = 0;
		// Called after all OnUpdate passes (e.g. for camera follow after physics)
		virtual void OnLateUpdate(Scene* scene,Timestep ts)
		{
		}
		virtual const char* GetName() const = 0;
		virtual int GetPriority() const = 0; // lower = runs first
	};
}