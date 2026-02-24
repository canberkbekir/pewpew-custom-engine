#pragma once

#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Components/TransformComponent.h"
#include "CBEngine/Components/RigidBodyComponent.h"
#include "CBEngine/Components/ColliderComponent.h"
#include "CBEngine/Components/CameraComponent.h"
#include "CBEngine/Components/MeshRendererComponent.h"
#include "CBEngine/Components/VoxelRendererComponent.h"
#include "CBEngine/Components/DirectionalLightComponent.h"
#include "CBEngine/Components/AudioSourceComponent.h"
#include "CBEngine/Components/HingeJointComponent.h"
#include "CBEngine/Physics/PhysicsWorld.h"

#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <glm/gtc/constants.hpp>

namespace CB
{
	// =========================================================================
	// RigidBodyProxy
	// =========================================================================
	struct RigidBodyProxy
	{
		Entity OwnerEntity;

		bool IsValid() const
		{
			return OwnerEntity && OwnerEntity.HasComponent<RigidBodyComponent>()
				&& OwnerEntity.GetComponent<RigidBodyComponent>().BodyCreated;
		}
		PhysicsWorld* GetWorld() const
		{
			Scene* scene = OwnerEntity.GetScene();
			return scene ? scene->GetPhysicsWorld() : nullptr;
		}
		JPH::BodyID GetBodyID() const
		{
			return OwnerEntity.GetComponent<RigidBodyComponent>().RuntimeBodyID;
		}
	};

	// =========================================================================
	// TransformProxy
	// =========================================================================
	struct TransformProxy
	{
		Entity OwnerEntity;

		bool IsValid() const
		{
			return OwnerEntity && OwnerEntity.HasComponent<TransformComponent>();
		}
	};

	// =========================================================================
	// CameraProxy
	// =========================================================================
	struct CameraProxy
	{
		Entity OwnerEntity;

		bool IsValid() const
		{
			return OwnerEntity && OwnerEntity.HasComponent<CameraComponent>();
		}
	};

	// =========================================================================
	// ColliderProxy
	// =========================================================================
	struct ColliderProxy
	{
		Entity OwnerEntity;

		bool IsValid() const
		{
			return OwnerEntity && OwnerEntity.HasComponent<ColliderComponent>();
		}
	};

	// =========================================================================
	// MeshRendererProxy
	// =========================================================================
	struct MeshRendererProxy
	{
		Entity OwnerEntity;

		bool IsValid() const
		{
			return OwnerEntity && OwnerEntity.HasComponent<MeshRendererComponent>();
		}
	};

	// =========================================================================
	// VoxelRendererProxy
	// =========================================================================
	struct VoxelRendererProxy
	{
		Entity OwnerEntity;

		bool IsValid() const
		{
			return OwnerEntity && OwnerEntity.HasComponent<VoxelRendererComponent>();
		}
	};

	// =========================================================================
	// DirectionalLightProxy
	// =========================================================================
	struct DirectionalLightProxy
	{
		Entity OwnerEntity;

		bool IsValid() const
		{
			return OwnerEntity && OwnerEntity.HasComponent<DirectionalLightComponent>();
		}
	};

	// =========================================================================
	// MeshProxy — wraps a Ref<Mesh>
	// =========================================================================
	struct MeshProxy
	{
		Ref<Mesh> MeshAsset;

		bool IsValid() const { return MeshAsset != nullptr; }
	};

	// =========================================================================
	// MaterialProxy — wraps the material on a MeshRendererComponent
	// =========================================================================
	struct MaterialProxy
	{
		Entity OwnerEntity;

		bool IsValid() const
		{
			return OwnerEntity && OwnerEntity.HasComponent<MeshRendererComponent>()
				&& OwnerEntity.GetComponent<MeshRendererComponent>().MaterialAsset != nullptr;
		}
	};

	// =========================================================================
	// AudioSourceProxy — wraps AudioSourceComponent
	// =========================================================================
	struct AudioSourceProxy
	{
		Entity OwnerEntity;

		bool IsValid() const
		{
			return OwnerEntity && OwnerEntity.HasComponent<AudioSourceComponent>();
		}
	};

	// =========================================================================
	// HingeJointProxy — wraps HingeJointComponent
	// =========================================================================
	struct HingeJointProxy
	{
		Entity OwnerEntity;

		bool IsValid() const
		{
			return OwnerEntity && OwnerEntity.HasComponent<HingeJointComponent>();
		}

		bool IsActive() const
		{
			if (!IsValid()) return false;
			auto& hj = OwnerEntity.GetComponent<HingeJointComponent>();
			return hj.ConstraintCreated && !hj.IsBroken;
		}

		float GetCurrentAngle() const
		{
			if (!IsValid()) return 0.0f;
			auto& hj = OwnerEntity.GetComponent<HingeJointComponent>();
			if (!hj.ConstraintCreated || !hj.RuntimeConstraint) return 0.0f;
			auto* c = static_cast<JPH::HingeConstraint*>(hj.RuntimeConstraint);
			return glm::degrees(c->GetCurrentAngle());
		}

		void SetMotorVelocity(float degsPerSec)
		{
			if (!IsValid()) return;
			auto& hj = OwnerEntity.GetComponent<HingeJointComponent>();
			hj.TargetVelocity = degsPerSec;
			if (hj.ConstraintCreated && hj.RuntimeConstraint)
			{
				auto* c = static_cast<JPH::HingeConstraint*>(hj.RuntimeConstraint);
				c->SetTargetAngularVelocity(glm::radians(degsPerSec));
			}
		}

		void SetMotorEnabled(bool on)
		{
			if (!IsValid()) return;
			auto& hj = OwnerEntity.GetComponent<HingeJointComponent>();
			hj.UseMotor = on;
			if (hj.ConstraintCreated && hj.RuntimeConstraint)
			{
				auto* c = static_cast<JPH::HingeConstraint*>(hj.RuntimeConstraint);
				c->SetMotorState(on ? JPH::EMotorState::Velocity : JPH::EMotorState::Off);
			}
		}

		void Break()
		{
			if (!IsValid()) return;
			auto& hj = OwnerEntity.GetComponent<HingeJointComponent>();

			// Remove the live Jolt constraint so the body is truly free immediately.
			if (hj.ConstraintCreated && hj.RuntimeConstraint)
			{
				Scene* scene = OwnerEntity.GetScene();
				PhysicsWorld* pw = scene ? scene->GetPhysicsWorld() : nullptr;
				if (pw)
				{
					pw->GetPhysicsSystem().RemoveConstraint(hj.RuntimeConstraint);
					hj.RuntimeConstraint->Release();
					hj.RuntimeConstraint = nullptr;
				}
			}

			hj.ConstraintCreated = true; // prevent HingeJointSystem from re-initialising
			hj.IsBroken          = true;
		}

		void Reset()
		{
			if (!IsValid()) return;
			auto& hj = OwnerEntity.GetComponent<HingeJointComponent>();
			hj.IsBroken = false;
			hj.ConstraintCreated = false;
			hj.RuntimeConstraint = nullptr;
		}
	};
}
