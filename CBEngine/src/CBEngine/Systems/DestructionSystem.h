#pragma once

#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Core/TimeStep.h"
#include "CBEngine/Core/UUID.h"
#include "CBEngine/Math/CoreMath.h"

#include <vector>

namespace CB
{
	struct DestructionRequest
	{
		UUID TargetEntity;
		Vector3 ImpactPoint;
		float DamageRadius = 1.0f;
		Vector3 ImpactForce = Vector3(0.0f);
	};

	class DestructionSystem
	{
	public:
		static void RequestDestruction(const DestructionRequest& request);
		static void OnUpdate(Scene* scene,Timestep ts);
	private:
		static void ProcessRequest(Scene* scene,const DestructionRequest& request);
		static std::vector<DestructionRequest> s_PendingRequests;
	};
}