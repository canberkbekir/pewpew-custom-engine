#pragma once
#include "CBEngine/Math/CoreMath.h"
#include "CBEngine/Scene/Scene.h"

namespace CB
{
	class TransformSystem
	{
	public: 
		static void OnUpdate(Scene* scene,Timestep ts);
	private:
		static void UpdateHierarchy(Scene* scene, Entity entity, const Mat4& parentWorld);
	};
	 
} 