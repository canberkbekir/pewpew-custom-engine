#pragma once

#include "imgui.h"
#include "CBEngine/Scene/Entity.h"
#include "CBEngine/Scene/Scene.h"
#include "CBEngine/Scene/SceneManager.h"
#include "CBEngine/Components/HingeJointComponent.h"
#include "CBEngine/Components/CoreComponents.h"
#include "CBEngine/Components/RigidBodyComponent.h"
#include "CBEngine/Physics/PhysicsWorld.h"
#include "../ComponentCard.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <glm/glm.hpp>

#include <cstdio>

namespace CB
{
	class HingeJointComponentView
	{
	public:
		static void Draw(Entity entity)
		{
			if (!entity.HasComponent<HingeJointComponent>())
				return;

			bool removed = false;
			bool reset = false;

			bool opened = ComponentCard::Begin("Hinge Joint", true, &removed, &reset);

			if (removed) {
				// Clean up runtime constraint before removing component
				auto& hj = entity.GetComponent<HingeJointComponent>();
				if (hj.ConstraintCreated && hj.RuntimeConstraint) {
					Scene* scene = entity.GetScene();
					if (scene && scene->GetPhysicsWorld()) {
						scene->GetPhysicsWorld()->GetPhysicsSystem()
						     .RemoveConstraint(hj.RuntimeConstraint);
						hj.RuntimeConstraint->Release();
						hj.RuntimeConstraint = nullptr;
						hj.ConstraintCreated = false;
					}
				}
				entity.RemoveComponent<HingeJointComponent>();
				ComponentCard::End();
				return;
			}

			if (reset) {
				auto& hj = entity.GetComponent<HingeJointComponent>();
				hj = HingeJointComponent{};
			}

			if (opened) {
				auto& hj = entity.GetComponent<HingeJointComponent>();

				// ---- Connected Body ----
				ImGui::TextUnformatted("Connected Body");
				ImGui::Separator();

				// Entity picker combo
				{
					auto DestroyRuntimeConstraint = [&]() {
						if (hj.RuntimeConstraint) {
							Scene* s = entity.GetScene();
							if (s && s->GetPhysicsWorld()) {
								s->GetPhysicsWorld()->GetPhysicsSystem()
								 .RemoveConstraint(hj.RuntimeConstraint);
								hj.RuntimeConstraint->Release();
								hj.RuntimeConstraint = nullptr;
							}
						}
						hj.ConstraintCreated = false;
					};

					Ref<Scene> scene = SceneManager::GetActiveScene();

					// Determine current label
					std::string currentLabel = "World (fixed)";
					if (hj.ConnectedBodyUUID != UUID(0) && scene) {
						Entity connected = scene->GetEntityByUUID(hj.ConnectedBodyUUID);
						if (connected && connected.HasComponent<TagComponent>())
							currentLabel = connected.GetComponent<TagComponent>().Tag;
						else
							currentLabel = "(entity not found)";
					}

					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() - 4.0f);
					if (ImGui::BeginCombo("##ConnectedBody", currentLabel.c_str())) {
						// Option: World (fixed)
						bool isWorld = (hj.ConnectedBodyUUID == UUID(0));
						if (ImGui::Selectable("World (fixed)", isWorld)) {
							hj.ConnectedBodyUUID = UUID(0);
							DestroyRuntimeConstraint();
						}

						ImGui::Separator();

						// List all entities with RigidBody (excluding self)
						if (scene) {
							auto view = scene->GetRegistry().view<TagComponent>();
							for (auto e : view) {
								Entity candidate{ e, scene.get() };
								if (candidate == entity) continue;
								if (!candidate.HasComponent<RigidBodyComponent>()) continue;

								const auto& tag = view.get<TagComponent>(e).Tag;
								UUID candidateUUID = candidate.GetUUID();
								bool selected = (hj.ConnectedBodyUUID == candidateUUID);

								ImGui::PushID(static_cast<int>(static_cast<uint32_t>((uint64_t)candidateUUID)));
								if (ImGui::Selectable(tag.c_str(), selected)) {
									hj.ConnectedBodyUUID = candidateUUID;
									DestroyRuntimeConstraint();
								}
								ImGui::PopID();
							}
						}
						ImGui::EndCombo();
					}

					// Accept entity drag-drop from hierarchy
					if (ImGui::BeginDragDropTarget()) {
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_ENTITY")) {
							UUID droppedUUID = *static_cast<const UUID*>(payload->Data);
							hj.ConnectedBodyUUID = droppedUUID;
							DestroyRuntimeConstraint();
						}
						ImGui::EndDragDropTarget();
					}

					ImGui::SameLine();
					if (ImGui::SmallButton("X##CB")) {
						hj.ConnectedBodyUUID = UUID(0);
						DestroyRuntimeConstraint();
					}
				}

				ImGui::Spacing();

				// ---- Anchor / Axis ----
				ImGui::TextUnformatted("Anchor & Axis");
				ImGui::Separator();

				if (ImGui::DragFloat3("Anchor##HJ", &hj.Anchor.x, 0.01f))
					hj.ConstraintCreated = false;
				if (ImGui::DragFloat3("Axis##HJ", &hj.Axis.x, 0.01f))
					hj.ConstraintCreated = false;

				ImGui::Checkbox("Auto Configure Connected Anchor", &hj.AutoConfigureConnectedAnchor);
				if (!hj.AutoConfigureConnectedAnchor) {
					if (ImGui::DragFloat3("Connected Anchor##HJ", &hj.ConnectedAnchor.x, 0.01f))
						hj.ConstraintCreated = false;
				}

				ImGui::Spacing();

				// ---- Limits ----
				ImGui::TextUnformatted("Limits");
				ImGui::Separator();

				if (ImGui::Checkbox("Use Limits##HJ", &hj.UseLimits))
					hj.ConstraintCreated = false;

				if (hj.UseLimits) {
					if (ImGui::DragFloat("Min (deg)##HJ", &hj.LimitsMin, 1.0f, -180.0f, 0.0f))
						hj.ConstraintCreated = false;
					if (ImGui::DragFloat("Max (deg)##HJ", &hj.LimitsMax, 1.0f, 0.0f, 180.0f))
						hj.ConstraintCreated = false;
					ImGui::DragFloat("Bounciness##HJ", &hj.Bounciness, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("Contact Distance##HJ", &hj.ContactDistance, 0.1f, 0.0f, 90.0f);
				}

				ImGui::Spacing();

				// ---- Spring ----
				ImGui::TextUnformatted("Spring");
				ImGui::Separator();

				if (ImGui::Checkbox("Use Spring##HJ", &hj.UseSpring))
					hj.ConstraintCreated = false;

				if (hj.UseSpring) {
					if (ImGui::DragFloat("Frequency (Hz)##HJ", &hj.SpringFrequency, 0.1f, 0.0f, 100.0f))
						hj.ConstraintCreated = false;
					if (ImGui::DragFloat("Damping##HJ", &hj.SpringDamping, 0.01f, 0.0f, 10.0f))
						hj.ConstraintCreated = false;
					ImGui::DragFloat("Target Position (deg)##HJ", &hj.TargetPosition, 1.0f, -180.0f, 180.0f);
				}

				ImGui::Spacing();

				// ---- Motor ----
				ImGui::TextUnformatted("Motor");
				ImGui::Separator();

				if (ImGui::Checkbox("Use Motor##HJ", &hj.UseMotor))
					hj.ConstraintCreated = false;

				if (hj.UseMotor) {
					ImGui::DragFloat("Target Velocity (deg/s)##HJ", &hj.TargetVelocity, 1.0f);
					if (ImGui::DragFloat("Motor Force (Nm)##HJ", &hj.MotorForce, 10.0f, 0.0f, 1e8f))
						hj.ConstraintCreated = false;
					ImGui::Checkbox("Free Spin##HJ", &hj.FreeSpin);
				}

				ImGui::Spacing();

				// ---- Break ----
				ImGui::TextUnformatted("Break");
				ImGui::Separator();

				ImGui::DragFloat("Break Force (N)##HJ", &hj.BreakForce, 1.0f, 0.0f, 1e8f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("0 = unbreakable");
				ImGui::DragFloat("Break Torque (Nm)##HJ", &hj.BreakTorque, 1.0f, 0.0f, 1e8f);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("0 = unbreakable");

				ImGui::Spacing();

				// ---- Options ----
				ImGui::TextUnformatted("Options");
				ImGui::Separator();

				if (ImGui::Checkbox("Enable Collision Between Bodies##HJ", &hj.EnableCollision))
					hj.ConstraintCreated = false;
				if (ImGui::DragFloat("Max Friction Torque (Nm)##HJ", &hj.MaxFrictionTorque, 0.1f, 0.0f, 1e6f))
					hj.ConstraintCreated = false;

				ImGui::Spacing();

				// ---- Runtime State ----
				ImGui::TextUnformatted("Runtime");
				ImGui::Separator();

				if (hj.ConstraintCreated && hj.RuntimeConstraint) {
					auto* c = static_cast<JPH::HingeConstraint*>(hj.RuntimeConstraint);
					float angleDeg = glm::degrees(c->GetCurrentAngle());
					ImGui::Text("Current Angle: %.1f deg", angleDeg);
				}
				else { ImGui::TextDisabled("Constraint not active"); }

				if (hj.IsBroken) {
					ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "BROKEN");
					ImGui::SameLine();
					if (ImGui::SmallButton("Repair##HJ")) {
						hj.IsBroken = false;
						hj.ConstraintCreated = false;
						hj.RuntimeConstraint = nullptr;
					}
				}
				else {
					if (ImGui::SmallButton("Reset Constraint##HJ")) {
						if (hj.ConstraintCreated && hj.RuntimeConstraint) {
							Scene* s = entity.GetScene();
							if (s && s->GetPhysicsWorld()) {
								s->GetPhysicsWorld()->GetPhysicsSystem()
								 .RemoveConstraint(hj.RuntimeConstraint);
								hj.RuntimeConstraint->Release();
								hj.RuntimeConstraint = nullptr;
							}
						}
						hj.ConstraintCreated = false;
					}
				}
			}

			ComponentCard::End();
		}
	};
}